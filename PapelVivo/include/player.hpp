#pragma once
#include "mpv_api.hpp"
#include "policy.hpp"
#include <atomic>
#include <GL/gl.h>
#include <string>
#include <thread>
#include <windows.h>

constexpr UINT WM_PV_MPV = WM_APP + 20, WM_PV_RENDER_ERROR = WM_APP + 21;
class Player {
    MpvApi api_;
    mpv_handle *core_ = nullptr;
    mpv_render_context *render_ = nullptr;
    HWND notify_ = nullptr, target_ = nullptr;
    HDC glDC_ = nullptr;
    HGLRC glContext_ = nullptr;
    static void *glProc(void *, const char *name);
    HANDLE stop_ = nullptr, update_ = nullptr, ready_ = nullptr;
    std::atomic<int> initialization_{MPV_ERROR_GENERIC};
    std::thread thread_;
    std::atomic<int> cap_{30};
    std::atomic<unsigned> width_{1}, height_{1};
    std::atomic<bool> redraw_{false};
    std::atomic<uint64_t> presents_{0};
    std::atomic<uint64_t> callbacks_{0};
    std::atomic<int> stage_{0};
    std::atomic<long long> waitDebug_{0};
    static void wakeCore(void *p) {
        auto s = static_cast<Player *>(p);
        PostMessageW(s->notify_, WM_PV_MPV, 0, 0);
    }
    static void wakeRender(void *p) {
        auto *self = static_cast<Player *>(p);
        self->callbacks_++;
        SetEvent(self->update_);
    }
    bool native_ = false;
    void run();

  public:
    ~Player() { stop(); }
    bool start(HWND notify, HWND target, const std::wstring &dll, int fps, int volume,
               std::wstring &error, bool software = false);
    void stop();
    bool active() const { return core_ != nullptr && IsWindow(target_); }
    bool nativeOutput() const { return native_; }
    bool outputReady() { return active() && text("vo-configured")=="yes"; }
    void load(const std::string &file);
    void pause(bool pause);
    void volume(int value);
    void fps(int value);
    void resize(unsigned w, unsigned h) {
        width_ = w ? w : 1;
        height_ = h ? h : 1;
        redraw_ = true;
        if (update_)
            SetEvent(update_);
    }
    int stage() const { return stage_; }
    long long waitDebug() const { return waitDebug_; }
    uint64_t callbacks() const { return callbacks_.load(); }
    uint64_t presents() const { return presents_.load(); }
    std::string text(const char *name);
    double number(const char *name);
    // Eventos são drenados somente no thread Win32, nunca no callback mpv.
    template <class F> void events(F handle) {
        if (!core_)
            return;
        while (auto *e = api_.mpv_wait_event(core_, 0)) {
            if (e->event_id == MPV_EVENT_NONE)
                break;
            handle(*e);
        }
    }
};
