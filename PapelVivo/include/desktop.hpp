#pragma once
#include "policy.hpp"
#include "workflow.hpp"
#include <windows.h>

struct DesktopTarget {
    enum Kind { None, ClassicWorker, RaisedProgman } kind = None;
    HWND progman = nullptr, parent = nullptr, icons = nullptr, background = nullptr;
    explicit operator bool() const { return kind != None && parent; }
};
inline bool raisedDesktop(HWND progman) {
    return IsWindow(progman) && pv::usesRaisedDesktop(GetWindowLongPtrW(progman, GWL_EXSTYLE));
}
inline RECT primaryMonitor() {
    POINT p{0, 0};
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY), &info);
    return info.rcMonitor;
}
inline bool coversPrimary(HWND window) {
    RECT r{}, m = primaryMonitor();
    return IsWindow(window) && IsWindowVisible(window) && GetWindowRect(window, &r) &&
           pv::covers({r.left, r.top, r.right, r.bottom}, {m.left, m.top, m.right, m.bottom});
}
inline bool sameShell(HWND h, HWND progman) {
    DWORD a = 0, b = 0;
    if (!IsWindow(h) || !IsWindow(progman))
        return false;
    GetWindowThreadProcessId(h, &a);
    GetWindowThreadProcessId(progman, &b);
    return a && a == b;
}
inline bool workerCandidate(HWND h, HWND progman, bool child) {
    if (!h || h == progman || FindWindowExW(h, nullptr, L"SHELLDLL_DefView", nullptr) ||
        !sameShell(h, progman) || GetParent(h) != (child ? progman : nullptr))
        return false;
    wchar_t cls[64]{};
    GetClassNameW(h, cls, 64);
    return wcscmp(cls, L"WorkerW") == 0 && coversPrimary(h);
}
inline DesktopTarget discoverDesktop(HWND progman) {
    if (!IsWindow(progman))
        return {};
    if (raisedDesktop(progman)) {
        // Nunca anexa ao WorkerW filho: ele é a camada estática, abaixo do vídeo.
        HWND icons = FindWindowExW(progman, nullptr, L"SHELLDLL_DefView", nullptr);
        if (!icons || !sameShell(icons, progman) || !IsWindowVisible(icons) ||
            !coversPrimary(progman))
            return {};
        HWND bg = nullptr;
        while ((bg = FindWindowExW(progman, bg, L"WorkerW", nullptr)))
            if (workerCandidate(bg, progman, true))
                break;
        return {DesktopTarget::RaisedProgman, progman, progman, icons, bg};
    }
    struct Search {
        HWND progman;
        DesktopTarget target;
    } search{progman, {}};
    EnumWindows(
        [](HWND h, LPARAM data) -> BOOL {
            auto &s = *reinterpret_cast<Search *>(data);
            HWND icons = FindWindowExW(h, nullptr, L"SHELLDLL_DefView", nullptr);
            if (icons && sameShell(h, s.progman) && sameShell(icons, s.progman)) {
                HWND next = h;
                while ((next = FindWindowExW(nullptr, next, L"WorkerW", nullptr)))
                    if (workerCandidate(next, s.progman, false)) {
                        s.target = {DesktopTarget::ClassicWorker, s.progman, next, icons, nullptr};
                        return FALSE;
                    }
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&search));
    return search.target;
}
inline DesktopTarget findDesktopTarget(HWND progman = FindWindowW(L"Progman", nullptr)) {
    if (!IsWindow(progman))
        return {};
    // Em raised desktop o 0x052C também prepara a composição transparente dos
    // ícones. Em 24H2, só localizar os HWNDs pode deixar DefView ainda opaco.
    bool prepareRaised = raisedDesktop(progman);
    if (!prepareRaised)
        if (auto target = discoverDesktop(progman))
            return target;
    DWORD_PTR reply = 0;
    for (int attempt = 0; attempt < 7; ++attempt) {
        const WPARAM wp = attempt % 3 == 0 ? 0 : 0xD;
        const LPARAM lp = attempt % 3 == 2 ? 1 : 0;
        SendMessageTimeoutW(progman, 0x052C, wp, lp, SMTO_ABORTIFHUNG | SMTO_BLOCK, 250, &reply);
        Sleep(40); // número limitado de tentativas; nunca gira em busy-wait
        if (auto target = discoverDesktop(progman))
            return target;
    }
    return {};
}
inline RECT childMonitorRect(HWND parent) {
    RECT r = primaryMonitor();
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT *>(&r), 2);
    return r;
}
inline bool targetStillValid(const DesktopTarget &t) {
    if (!t || !IsWindow(t.progman) || !coversPrimary(t.parent))
        return false;
    if (t.kind == DesktopTarget::RaisedProgman)
        return raisedDesktop(t.progman) && t.parent == t.progman && IsWindow(t.icons) &&
               GetParent(t.icons) == t.progman && sameShell(t.icons, t.progman) &&
               IsWindowVisible(t.icons) &&
               FindWindowExW(t.progman, nullptr, L"SHELLDLL_DefView", nullptr) == t.icons;
    return !raisedDesktop(t.progman) && workerCandidate(t.parent, t.progman, false) &&
           IsWindow(t.icons) && sameShell(t.icons, t.progman);
}
inline bool layeredStylesValid(HWND video) {
    const auto style = GetWindowLongPtrW(video, GWL_STYLE);
    const auto ex = GetWindowLongPtrW(video, GWL_EXSTYLE);
    const auto cls = GetClassLongPtrW(video, GCL_STYLE);
    return (style & WS_CHILD) && !(style & WS_POPUP) && (ex & WS_EX_LAYERED) &&
           !(ex & WS_EX_NOREDIRECTIONBITMAP) && !(cls & (CS_OWNDC | CS_CLASSDC | CS_PARENTDC));
}
inline bool beforeSibling(HWND first, HWND other) {
    if (GetParent(first) != GetParent(other))
        return false;
    for (HWND h = GetWindow(first, GW_HWNDNEXT); h; h = GetWindow(h, GW_HWNDNEXT))
        if (h == other)
            return true;
    return false;
}
inline bool positionWallpaper(HWND video, const DesktopTarget &t) {
    if (!targetStillValid(t) || GetParent(video) != t.parent)
        return false;
    HWND after = HWND_BOTTOM;
    if (t.kind == DesktopTarget::RaisedProgman) {
        if (!layeredStylesValid(video))
            return false;
        after = t.icons; // filho direto, imediatamente DEPOIS de DefView na ordem Z
    }
    RECT r = childMonitorRect(t.parent);
    if (!SetWindowPos(video, after, r.left, r.top, r.right - r.left, r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_SHOWWINDOW))
        return false;
    // Não altera a ordem Z de janelas do Explorer; valida a camada resultante.
    return t.kind != DesktopTarget::RaisedProgman || !IsWindow(t.background) ||
           beforeSibling(video, t.background);
}
inline bool bindWallpaper(HWND video, const DesktopTarget &t) {
    const auto style = GetWindowLongPtrW(video, GWL_STYLE);
    if (!targetStillValid(t) || !(style & WS_CHILD) || (style & WS_POPUP))
        return false;
    SetLastError(0);
    HWND previous = SetParent(video, t.parent);
    if ((!previous && GetLastError() != 0) || GetParent(video) != t.parent)
        return false;
    if (t.kind == DesktopTarget::RaisedProgman &&
        (!layeredStylesValid(video) || !SetLayeredWindowAttributes(video, 0, 0xFF, LWA_ALPHA)))
        return false;
    return positionWallpaper(video, t);
}
inline bool attachedWallpaper(HWND video, const DesktopTarget &t) {
    const auto style = GetWindowLongPtrW(video, GWL_STYLE);
    if (!targetStillValid(t) || GetParent(video) != t.parent || !coversPrimary(video) ||
        !(style & WS_CHILD) || (style & WS_POPUP))
        return false;
    if (t.kind == DesktopTarget::RaisedProgman) {
        BYTE alpha = 0;
        DWORD flags = 0;
        COLORREF key = 0;
        return layeredStylesValid(video) &&
               GetLayeredWindowAttributes(video, &key, &alpha, &flags) && (flags & LWA_ALPHA) &&
               alpha == 0xFF && GetWindow(video, GW_HWNDPREV) == t.icons &&
               (!IsWindow(t.background) || beforeSibling(video, t.background));
    }
    return true;
}
// Sobreposição é fallback explícito, NÃO é o modo padrão atrás dos ícones.
inline bool sinkAlternative(HWND video) {
    if (!IsWindow(video))
        return false;
    HWND anchor = nullptr;
    for (HWND h = GetTopWindow(nullptr); h; h = GetWindow(h, GW_HWNDNEXT)) {
        if (h == video || !IsWindowVisible(h))
            continue;
        wchar_t cls[64]{};
        GetClassNameW(h, cls, 64);
        if (wcscmp(cls, L"Progman") == 0 || wcscmp(cls, L"WorkerW") == 0) {
            anchor = h;
            break;
        }
    }
    HWND after = anchor ? GetWindow(anchor, GW_HWNDPREV) : HWND_BOTTOM;
    if (after == video)
        return true;
    if (!after || (after != HWND_BOTTOM && (GetWindowLongPtrW(after, GWL_EXSTYLE) & WS_EX_TOPMOST)))
        after = HWND_TOP;
    RECT r = primaryMonitor();
    return SetWindowPos(video, after, r.left, r.top, r.right - r.left, r.bottom - r.top,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW) != FALSE;
}
