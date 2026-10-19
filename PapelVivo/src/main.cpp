#include "desktop.hpp"
#include "language.hpp"
#include "media_job.hpp"
#include "player.hpp"
#include "catalog.hpp"
#include "local_worker.hpp"
#include <algorithm>
#include <chrono>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <filesystem>
#include <fstream>
#include <gdiplus.h>
#include <initguid.h>
#include <iomanip>
#include <knownfolders.h>
#if __has_include(<powersetting.h>)
#include <powersetting.h>
#endif
#include <powrprof.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <uxtheme.h>
#include <vector>
#include <windows.h>
#include <wtsapi32.h>

namespace {
constexpr UINT WM_TRAY = WM_APP + 1, WM_CHECK = WM_APP + 2;
enum {
  ID_CHOOSE = 100,
  ID_APPLY,
  ID_PREVIEW,
  ID_PAUSE,
  ID_STOP,
  ID_ORIGINAL,
  ID_FPS,
  ID_AUTO,
  ID_VOLUME,
  ID_SHOW,
  ID_EXIT,
  ID_ABOUT,
  ID_ALTERNATIVE,
  ID_LOGS,
  ID_FILES,
  ID_LANGUAGE,
  ID_GALLERY=117, ID_REMOVE_BG=118, ID_SAVE_CLOSE=119,
  ID_PT = 2001,
  ID_EN = 2002
};
HINSTANCE instance;
HWND window = nullptr, video = nullptr, worker = nullptr, fileBox = nullptr,
     statusBox = nullptr, detailBox = nullptr, fpsBox = nullptr,
     autoBox = nullptr, volumeBox = nullptr, pauseButton = nullptr;
HFONT normalFont = nullptr, titleFont = nullptr;
HBRUSH bg = nullptr;
Player player;
DesktopTarget desktopTarget;
MediaJob media,thumbnailJob;
enum class Intent { Select, Desktop, Preview, Alternative };
Intent pendingIntent = Intent::Select;
bool converting = false, alternative = false, applicationPending = false,
     fileLoaded = false;
ULONGLONG applicationStarted = 0;
uint64_t loadedFrameMark = 0;
std::wstring checkedPath, convertedFolder;
WIN32_FILE_ATTRIBUTE_DATA checkedStamp{};
pv::VideoSize checkedSize;
void requestVideo(Intent intent);
std::wstring posterFileFor(const std::wstring&);
void removeVideo();
void updateUi();void openGallery();void catalogSave();void catalogAdd(const std::wstring&);RECT previewBounds();void layoutControls();void refreshLanguage();void detectHardware();bool isStillImage(const std::wstring&);bool shouldOptimize(const pv::VideoSize&);void performanceWarning();void queueThumbnails();void pollThumbnails();void thumbnailFinished();void startRemoval();void pollRemoval();
LocalWorker imageTask;
std::wstring activePath;
void signalShutdown(){wchar_t name[256]{};if(GetEnvironmentVariableW(L"PAPELVIVO_STOP_EVENT",name,256)){HANDLE e=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);if(e){SetEvent(e);CloseHandle(e);}}}

void apply(bool asPreview);
void useOverlay(bool manual);
pv::PausePolicy policy;
std::wstring baseDir, iniPath, selected, lastError;
std::wstring message = L"Seu papel de parede original está preservado.";
std::wstring renderInfo = L"Recomendado: H.264, 720p/1080p, até 30 FPS.";
int fps = 30, volume = 0;
bool preview = false, playing = false, lastPause = false, quitting = false,
     trayAdded = false;
HWINEVENTHOOK foregroundHook = nullptr;
HPOWERNOTIFY powerNotify = nullptr;
UINT taskbarCreated = 0;
uint64_t lastPresents = 0;
ULONGLONG lastTelemetry = 0, lastRecover = 0;
double actualFps = 0, cpu = 0;
unsigned long long lastCpu = 0, lastWall = 0;
NOTIFYICONDATAW tray{sizeof(tray)};
bool diagnosticSoftware = false;
bool selftest = false,testWarmed=false;

ULONGLONG testStart = 0;
double testPausePos=0,testPauseDrift=0,testResumePos=0;bool testProgressed=false;
uint64_t pausedFrames = 0, pauseStable = 0, pauseDrift = 0, resumeMark = 0;
bool testPaused = false, testResumed = false, pauseMeasured = false,
     testPassed = false;
std::wstring fromUtf8(const std::string &s) {
  if (s.empty())
    return {};
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                              nullptr, 0);
  std::wstring r(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                      r.data(), n);
  return r;
}
std::string toUtf8(const std::wstring &s) {
  int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                              nullptr, 0, nullptr, nullptr);
  std::string r(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                      r.data(), n, nullptr, nullptr);
  return r;
}
std::wstring moduleDir() {
  std::vector<wchar_t> b(32768);
  DWORD n = GetModuleFileNameW(nullptr, b.data(), static_cast<DWORD>(b.size()));
  std::wstring p(b.data(), n);
  return p.substr(0, p.find_last_of(L"\\/"));
}
struct Settings {std::wstring file;int rate=30,sound=0;bool automatic=true;bool operator==(const Settings&o)const{return file==o.file&&rate==o.rate&&sound==o.sound&&automatic==o.automatic;}};
Settings savedSettings;bool settingsReady=false,restoringSettings=false;std::vector<Settings> settingsUndo,settingsRedo;
Settings captureSettings(){return {selected,fps,volume,policy.automatic};}
void save() {
  auto now=captureSettings();if(settingsReady&&!restoringSettings&&!(now==savedSettings)){settingsUndo.push_back(savedSettings);settingsRedo.clear();}savedSettings=now;settingsReady=true;
  WritePrivateProfileStringW(L"Preferencias", L"FPS",
                             std::to_wstring(fps).c_str(), iniPath.c_str());
  WritePrivateProfileStringW(L"Preferencias", L"AutoPausa",
                             policy.automatic ? L"1" : L"0", iniPath.c_str());
  WritePrivateProfileStringW(L"Preferencias", L"Volume",
                             std::to_wstring(volume).c_str(), iniPath.c_str());
  WritePrivateProfileStringW(L"Preferencias", L"Video", selected.c_str(),
                             iniPath.c_str());
}
void setupPreferences() {
  PWSTR p = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE,
                                     nullptr, &p))) {
    std::wstring d = std::wstring(p) + L"\\PapelVivo\\Player";
    CoTaskMemFree(p);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(d), ec);
    iniPath = d + L"\\config.ini";
  } else
    iniPath = baseDir + L"\\config.ini";
  fps =
      GetPrivateProfileIntW(L"Preferencias", L"FPS", 30, iniPath.c_str()) == 60
          ? 60
          : 30;
  volume = std::clamp<int>(
      GetPrivateProfileIntW(L"Preferencias", L"Volume", 0, iniPath.c_str()), 0,
      100);
  policy.automatic = GetPrivateProfileIntW(L"Preferencias", L"AutoPausa", 1,
                                           iniPath.c_str()) != 0;
  std::vector<wchar_t> b(32768);
  GetPrivateProfileStringW(L"Preferencias", L"Video", L"", b.data(),
                           static_cast<DWORD>(b.size()), iniPath.c_str());
  selected = b.data();
  wchar_t language[16]{};
  GetPrivateProfileStringW(L"Preferencias", L"Idioma", L"pt-BR", language, 16,
                           iniPath.c_str());
  pvlang::english = wcscmp(language, L"en") == 0;
}
#include "ui_surface.hpp"
#include "ui_icons.hpp"
#include "panel.hpp"
#include "player_companion.hpp"
void error(const std::wstring &s, bool dialog = true) {
  recordLog(L"ERRO", s);
  lastError = s;
  std::wstring lower=s;std::transform(lower.begin(),lower.end(),lower.begin(),::towlower);
  std::wstring hint;if(lower.find(L"workerw")!=std::wstring::npos)hint=L"Falha na área atrás dos ícones. Pare e use o modo alternativo.";else if(lower.find(L"mpv")!=std::wstring::npos||lower.find(L"codec")!=std::wstring::npos)hint=L"Falha ao abrir ou decodificar a mídia. Confira o arquivo e use uma cópia H.264 mais leve.";else if(lower.find(L"opengl")!=std::wstring::npos)hint=L"Falha no renderizador. Atualize o driver ou tente sem aceleração.";
  if(!hint.empty())lastError=hint+L"\r\nDetalhe C++: "+s;
  message = s.size() > 130 ? s.substr(0, 127) + L"..." : s;
  if (statusBox)
    SetWindowTextW(statusBox, lastError.c_str());
  if(dialog&&!selftest) friendlyNotice(pvlang::text(s), pvlang::english?L"What happened":L"O que aconteceu");
}
#include "player_gallery.hpp"
#include "player_tools.hpp"
void updateUi() {
  if (!window)
    return;
  auto textIfChanged = [](HWND c, const std::wstring &text) {
    int n = GetWindowTextLengthW(c);
    std::wstring old(n + 1, L'\0');
    GetWindowTextW(c, old.data(), n + 1);
    old.resize(n);
    const auto display = c == fileBox ? text : pvlang::text(text);
    if (old != display)
      SetWindowTextW(c, display.c_str());
  };
  textIfChanged(fileBox,
                selected.empty() ? L"Nenhum vídeo selecionado" : selected);
  textIfChanged(statusBox, message);
  textIfChanged(detailBox, renderInfo);
  textIfChanged(pauseButton, policy.manual ? L"Continuar" : L"Pausar");
  EnableWindow(pauseButton, playing);
  EnableWindow(GetDlgItem(window, ID_STOP), TRUE);
  EnableWindow(GetDlgItem(window, ID_REMOVE_BG), !media.busy()&&!imageTask.busy());
  EnableWindow(GetDlgItem(window, ID_CHOOSE), !media.busy());
  EnableWindow(GetDlgItem(window, ID_APPLY), !media.busy());
  EnableWindow(GetDlgItem(window, ID_PREVIEW), !media.busy());
  EnableWindow(GetDlgItem(window, ID_ALTERNATIVE), !media.busy());
  EnableWindow(GetDlgItem(window, ID_ORIGINAL), playing);
  static bool wasPlaying = false, wasPreview = false;
  if (wasPlaying != playing || wasPreview != preview) {
    RECT r=previewBounds();
    InvalidateRect(window, &r, FALSE);
    wasPlaying = playing;
    wasPreview = preview;
  }
}
void undoSettings(bool redo){
 if(playing||media.busy()||imageTask.busy()){message=L"Pare a reprodução e o processamento antes de desfazer preferências.";updateUi();return;}
 auto&from=redo?settingsRedo:settingsUndo;auto&to=redo?settingsUndo:settingsRedo;if(from.empty())return;to.push_back(captureSettings());auto st=from.back();from.pop_back();selected=st.file;fps=st.rate;volume=st.sound;policy.automatic=st.automatic;restoringSettings=true;save();restoringSettings=false;SendMessageW(fpsBox,CB_SETCURSEL,fps==60?1:0,0);SendMessageW(autoBox,BM_SETCHECK,policy.automatic?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(volumeBox,TBM_SETPOS,TRUE,volume);updateUi();
}
void reconcilePause() {
  bool hiddenPreview =
      preview && (!IsWindowVisible(window) || IsIconic(window));
  bool value = policy.paused() || hiddenPreview || converting;
  if (playing && value != lastPause) {
    player.pause(value);
    lastPause = value;
  }
  if (playing) {
    if (converting)
      message = L"Convertendo para 1080p/30. A interface continua ativa; Parar "
                L"encerra e cancela.";
    else if (policy.locked)
      message = L"Pausado: sessão bloqueada.";
    else if (policy.suspended || policy.displayOff)
      message = L"Pausado: tela desligada ou suspensão.";
    else if (policy.manual)
      message = L"Pausado por você. Use Continuar para retomar.";
    else if (policy.automatic && policy.fullscreen)
      message = L"Pausado automaticamente: aplicativo em tela cheia.";
    else if (hiddenPreview)
      message = L"Prévia pausada: janela oculta ou minimizada.";
    else
      message = preview
                    ? L"Pré-visualização em janela — ainda não é o wallpaper."
                : applicationPending
                    ? L"Preparando vídeo e verificando encaixe no desktop..."
                : alternative ? L"Seu Windows bloqueou. Usando modo de "
                                L"sobreposição (não interativo)."
                              : L"Aplicado atrás dos ícones.";
  }
  updateUi();
}
void removeVideo() {
  playing = false;activePath.clear();
  player.stop();
  if (video && IsWindow(video))
    DestroyWindow(video);
  video = nullptr;
  if (worker && IsWindow(worker))
    RedrawWindow(worker, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
  worker = nullptr;
  desktopTarget = {};
  preview = false;
  alternative = applicationPending = fileLoaded = false;
  lastPause = false;
  policy.manual = false;
  lastPresents = 0;
  actualFps = 0;
  message = L"Parado. O papel de parede estático do Windows está visível.";
  renderInfo = L"Nenhum vídeo sendo decodificado.";
  updateUi();
}
LRESULT CALLBACK VideoProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_SIZE) {
    player.resize(LOWORD(l), HIWORD(l));
    return 0;
  }
  if (m == WM_ERASEBKGND)
    return 1;
  if (m == WM_NCHITTEST)
    return HTTRANSPARENT;
  if (m == WM_MOUSEACTIVATE)
    return MA_NOACTIVATE;
  return DefWindowProcW(h, m, w, l);
}
bool validFile(const std::wstring &f) {
  DWORD a = GetFileAttributesW(f.c_str());
  if (a == INVALID_FILE_ATTRIBUTES || (a & FILE_ATTRIBUTE_DIRECTORY))
    return false;
  auto p = f.find_last_of(L'.');
  if (p == std::wstring::npos)
    return false;
  auto ext = f.substr(p);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  return ext == L".mp4" || ext == L".webm" || ext == L".mkv" ||
         ext == L".avi" || ext == L".mov" || ext == L".m4v" || ext==L".png" || ext==L".jpg" || ext==L".jpeg" || ext==L".webp";
}
void startPlayback(bool asPreview, bool forceAlternative = false) {
  if (!validFile(selected)) {
    error(L"Escolha um arquivo local MP4, WebM, MKV, AVI ou MOV que exista no "
          L"disco.");
    return;
  }
  const bool manualBefore = policy.manual;
  DesktopTarget target =
      (asPreview || forceAlternative) ? DesktopTarget{} : findDesktopTarget();
  HWND destination = asPreview ? window : target.parent;
  const bool useAlternative = !asPreview && !destination;
  removeVideo();
  preview = asPreview;
  alternative = useAlternative;
  worker = (asPreview || useAlternative) ? nullptr : destination;
  desktopTarget = target;
  const bool layered =
      useAlternative || target.kind == DesktopTarget::RaisedProgman;
  RECT r = asPreview        ? previewBounds()
           : useAlternative ? primaryMonitor()
                            : childMonitorRect(destination);
  DWORD ex = WS_EX_NOACTIVATE | (layered ? WS_EX_LAYERED : 0);
  DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  if (useAlternative) {
    ex |= WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED;
    style |= WS_POPUP;
  } else
    style |= WS_CHILD;
  video = CreateWindowExW(
      ex, layered ? L"PapelVivo.VideoLayered" : L"PapelVivo.Video", L"", style,
      r.left, r.top, r.right - r.left, r.bottom - r.top,
      useAlternative ? nullptr : destination, nullptr, instance, nullptr);
  if (!video) {
    error(L"Não foi possível criar a superfície de vídeo. Código Windows: " +
          std::to_wstring(GetLastError()));
    return;
  }
  if (!asPreview && !useAlternative && !bindWallpaper(video, desktopTarget)) {
    removeVideo();
    useOverlay(manualBefore);
    return;
  }
  if (useAlternative && !SetLayeredWindowAttributes(video, 0, 255, LWA_ALPHA)) {
    removeVideo();
    error(L"O Windows recusou o modo alternativo. Seu wallpaper original foi "
          L"preservado.");
    return;
  }
  ShowWindow(video, SW_SHOWNOACTIVATE);
  if (!asPreview && !useAlternative &&
      !attachedWallpaper(video, desktopTarget)) {
    removeVideo();
    useOverlay(manualBefore);
    return;
  }
  if (useAlternative && !sinkAlternative(video)) {
    removeVideo();
    error(L"Não foi possível posicionar o modo alternativo. O original foi "
          L"preservado.");
    return;
  }
  std::wstring e;
  if (!player.start(window, video, baseDir + L"\\libmpv-2.dll", fps, volume, e,
                    diagnosticSoftware)) {
    removeVideo();
    error(e);
    return;
  }
  playing = true;activePath=selected;catalogAdd(selected);
  applicationPending = false;
  applicationStarted = 0;
  fileLoaded = false;
  if(useAlternative&&!selftest)companionTip(pvlang::english?L"Windows did not allow attachment behind icons. Alternative mode can cover them. Restore original to undo.":L"O Windows não permitiu encaixe atrás dos ícones. O modo alternativo pode cobri-los. Voltar ao original desfaz.");
  lastPause = !policy.paused();
  lastError.clear();
  applicationStarted=GetTickCount64();applicationPending=!asPreview;
  player.load(toUtf8(selected));
  reconcilePause();
  save();
  lastTelemetry = GetTickCount64();
  lastPresents = 0;
  if (selftest)
    testStart = GetTickCount64();
}

void useOverlay(bool manual) {
  startPlayback(false, true);
  policy.manual = manual;
  reconcilePause();
}

bool sameStamp(const WIN32_FILE_ATTRIBUTE_DATA &a,
               const WIN32_FILE_ATTRIBUTE_DATA &b) {
  return a.nFileSizeHigh == b.nFileSizeHigh &&
         a.nFileSizeLow == b.nFileSizeLow &&
         CompareFileTime(&a.ftLastWriteTime, &b.ftLastWriteTime) == 0;
}
void cacheSize(const std::wstring &path, pv::VideoSize size) {
  checkedPath = path;
  checkedSize = size;
  GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &checkedStamp);
}
void dispatchReady(Intent intent) {
  if (intent == Intent::Select) {
    message = L"Vídeo verificado. Aplique no desktop ou pré-visualize.";
    updateUi();
  } else
    startPlayback(intent == Intent::Preview, intent == Intent::Alternative);
}
bool realPlaybackActive(){
 if(playing&&(!IsWindow(video)||!player.active()||(fileLoaded&&player.text("idle-active")=="yes")))removeVideo();
 return playing&&IsWindow(video)&&player.active();
}
void requestVideo(Intent intent) {
  if(media.busy()||imageTask.busy())return;
  realPlaybackActive();
  if (!validFile(selected)) {
    error(L"Escolha um vídeo local válido antes de aplicar.");
    return;
  }
  WIN32_FILE_ATTRIBUTE_DATA stamp{};
  if (checkedPath == selected &&
      GetFileAttributesExW(selected.c_str(), GetFileExInfoStandard, &stamp) &&
      sameStamp(stamp, checkedStamp)) {
    dispatchReady(intent);
    return;
  }
  pendingIntent = intent;
  message = L"Verificando resolução antes de abrir o vídeo... Parar encerra o "
            L"aplicativo.";
  media.probe(window, baseDir + L"\\ferramentas", selected);
  updateUi();
}
void apply(bool asPreview) {
  
  requestVideo(asPreview ? Intent::Preview : Intent::Desktop);
}
void mediaFinished() {
  auto result = media.take();
  converting = false;
  reconcilePause();
  if (result.input != selected) return;
  if (result.kind == MediaResult::Cancelled) {message=L"Operação cancelada; original preservado.";updateUi();return;}
  if (result.kind == MediaResult::Failed) {
    recordLog(L"ERRO FFMPEG",result.error);
    if(validFile(selected)&&friendlyConfirm(pvlang::english?L"Video optimization failed. Try the original? It may overload your computer.":L"Não foi possível otimizar o vídeo. Tentar o original? Ele pode sobrecarregar seu computador.",pvlang::english?L"Optimization unavailable":L"Otimização indisponível")){cacheSize(selected,result.size);dispatchReady(pendingIntent);return;}
    message=L"Reprodução não autorizada. Original preservado.";updateUi();return;
  }
  if(result.kind==MediaResult::Probed && !isStillImage(selected) && shouldOptimize(result.size)){
    converting=true;companionTip(pvlang::english?L"This video is heavy for your hardware. Preparing a lighter copy; your original stays intact.":L"Este vídeo é pesado para seu hardware. Vou preparar uma cópia leve; o original fica intacto.");
    message=pvlang::english?L"Optimizing video… Stop cancels and exits.":L"Otimizando vídeo… Parar cancela e sai.";
    media.convert(window,baseDir+L"\\ferramentas",selected,convertedFolder);reconcilePause();updateUi();return;
  }
  if(result.kind==MediaResult::Converted){selected=result.output;catalogAdd(selected);if(!playerCatalog.items.empty()){auto name=std::filesystem::path(result.input).stem().u8string()+toUtf8(tr(L" (leve)",L" (optimized)"));playerCatalog.items.back().name=playerCatalog.unique(name);catalogSave();}save();}
  cacheSize(selected, result.size);
  dispatchReady(pendingIntent);
}
void choose() {
  

  if (media.busy())
    return;
  std::vector<wchar_t> b(32768);
  if (selected.size() < b.size())
    std::copy(selected.begin(), selected.end(), b.begin());
  OPENFILENAMEW f{sizeof(f)};
  f.hwndOwner = window;
  f.lpstrFile = b.data();
  f.nMaxFile = static_cast<DWORD>(b.size());
  f.lpstrFilter =
      pvlang::english
          ? L"Videos and images\0*.mp4;*.m4v;*.webm;*.mkv;*.avi;*.mov;*.png;*.jpg;*.jpeg;*.webp\0All "
            L"files\0*.*\0\0"
          : L"Vídeos e imagens\0*.mp4;*.m4v;*.webm;*.mkv;*.avi;*.mov;*.png;*.jpg;*.jpeg;*.webp\0Todos os "
            L"arquivos\0*.*\0\0";
  f.lpstrTitle = pvlang::english ? L"Choose a video for Papel Vivo"
                                 : L"Escolher vídeo para o Papel Vivo";
  f.Flags =
      OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
  if (GetOpenFileNameW(&f)) {
    selected = b.data();
    catalogAdd(selected);save();
    message = L"Vídeo selecionado. Aplique no desktop ou pré-visualize.";
    updateUi();
    requestVideo(Intent::Select);
  }
}
bool fullscreenActive() {
  HWND fg = GetForegroundWindow();
  if (!fg || fg == window || fg == video || fg == GetDesktopWindow() ||
      fg == GetShellWindow() || IsIconic(fg) || !IsWindowVisible(fg))
    return false;
  DWORD pid = 0;
  GetWindowThreadProcessId(fg, &pid);
  if (pid == GetCurrentProcessId())
    return false;
  wchar_t cls[128]{};
  GetClassNameW(fg, cls, 128);
  if (wcscmp(cls, L"Progman") == 0 || wcscmp(cls, L"WorkerW") == 0 ||
      wcscmp(cls, L"Shell_TrayWnd") == 0 ||
      wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0)
    return false;
  DWORD cloaked = 0;
  DwmGetWindowAttribute(fg, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
  if (cloaked)
    return false;
  RECT r{};
  if (FAILED(DwmGetWindowAttribute(fg, DWMWA_EXTENDED_FRAME_BOUNDS, &r,
                                   sizeof(r))))
    if (!GetWindowRect(fg, &r))
      return false;
  MONITORINFO info{sizeof(info)};
  if (!GetMonitorInfoW(MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST), &info))
    return false;
  auto m = info.rcMonitor;
  return pv::covers({r.left, r.top, r.right, r.bottom},
                    {m.left, m.top, m.right, m.bottom});
}
void checkEnvironment() {
  policy.sample(fullscreenActive(), static_cast<int64_t>(GetTickCount64()));
  reconcilePause();
}
void CALLBACK focusEvent(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
  if (window)
    PostMessageW(window, WM_CHECK, 0, 0);
}
void addTray() {
  tray.hWnd = window;
  tray.uID = 1;
  tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  tray.uCallbackMessage = WM_TRAY;
  tray.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
  wcscpy_s(tray.szTip,
           pvlang::text(L"Papel Vivo — vídeo leve no desktop").c_str());
  trayAdded = Shell_NotifyIconW(NIM_ADD, &tray) != FALSE;
  tray.uVersion = NOTIFYICON_VERSION_4;
  if (trayAdded)
    Shell_NotifyIconW(NIM_SETVERSION, &tray);
}
void show() {
  ShowWindow(window, SW_RESTORE);
  SetForegroundWindow(window);
}
void menu() {
  HMENU m = CreatePopupMenu();
  AppendMenuW(m, MF_STRING, ID_SHOW, L"Abrir Papel Vivo");
  AppendMenuW(m, MF_STRING, ID_APPLY, L"Aplicar wallpaper");
  AppendMenuW(m, MF_STRING, ID_CHOOSE, L"Trocar vídeo...");
  AppendMenuW(m, MF_STRING | (playing ? 0 : MF_GRAYED), ID_PAUSE,
              policy.manual ? L"Continuar" : L"Pausar");
  AppendMenuW(m, MF_STRING, ID_STOP, L"Parar e encerrar");
  AppendMenuW(m, MF_STRING, ID_ALTERNATIVE, L"Modo alternativo");
  AppendMenuW(m, MF_STRING, ID_ORIGINAL, L"Voltar ao original");
  AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(m, MF_STRING, ID_SHOW, L"Configurações...");
  AppendMenuW(m, MF_STRING, ID_ABOUT, L"Sobre e desempenho");
  AppendMenuW(m, MF_STRING, ID_LOGS, L"Diagnóstico / copiar logs");
  AppendMenuW(m, MF_STRING, ID_FILES, L"Arquivos, README e licenças");
  AppendMenuW(m, MF_STRING, ID_LANGUAGE, L"Idioma");
  AppendMenuW(m, MF_STRING, ID_EXIT, L"Sair");
  POINT p;
  GetCursorPos(&p);
  SetForegroundWindow(window);
  int cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0,
                           window, nullptr);
  DestroyMenu(m);
  if (cmd)
    PostMessageW(window, WM_COMMAND, cmd, 0);
  PostMessageW(window, WM_NULL, 0, 0);
}
void telemetry() {
  const auto now = GetTickCount64();
  uint64_t count = player.presents();
  if (lastTelemetry && now > lastTelemetry)
    actualFps = static_cast<double>(count - lastPresents) * 1000 /
                (now - lastTelemetry);
  lastTelemetry = now;
  lastPresents = count;
  FILETIME create, exit, kernel, user, wall;
  GetSystemTimeAsFileTime(&wall);
  auto value = [](FILETIME t) {
    return (static_cast<unsigned long long>(t.dwHighDateTime) << 32) |
           t.dwLowDateTime;
  };
  if (GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user)) {
    auto c = value(kernel) + value(user), t = value(wall);
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    if (lastWall && t > lastWall)
      cpu = 100.0 * (c - lastCpu) / (t - lastWall) /
            std::max<DWORD>(1, si.dwNumberOfProcessors);
    lastCpu = c;
    lastWall = t;
  }
  PROCESS_MEMORY_COUNTERS_EX mem{};
  mem.cb = sizeof(mem);
  GetProcessMemoryInfo(GetCurrentProcess(),
                       reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&mem),
                       sizeof(mem));
  performanceWarning();updateUi();
}
LRESULT CALLBACK FocusReveal(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){if(m==WM_SETFOCUS)PostMessageW(GetParent(h),WM_APP+94,(WPARAM)h,0);if(m==WM_NCDESTROY)RemoveWindowSubclass(h,FocusReveal,9);return DefSubclassProc(h,m,w,l);}
HWND control(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y,
             int w, int h, int id = 0) {
  bool button = std::wstring(cls) == L"BUTTON" &&
                (style & BS_TYPEMASK) != BS_AUTOCHECKBOX;
  if (button)
    style = (style & ~BS_TYPEMASK) | BS_OWNERDRAW;
  HWND c = CreateWindowExW(
      0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, window,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
  SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
  SetWindowSubclass(c,FocusReveal,9,0);
  if (button)
    SetWindowSubclass(c, ButtonMotion, 1, 0);
  if (std::wstring(cls) == L"BUTTON" && !button)
    SetWindowTheme(c, L"", L"");
  return c;
}
void initControls(){
 fileBox=control(L"EDIT",L"",ES_READONLY|ES_AUTOHSCROLL|WS_TABSTOP,0,0,1,1);
 for(auto id:{ID_CHOOSE,ID_APPLY,ID_PREVIEW,ID_ALTERNATIVE,ID_LOGS,ID_ABOUT,ID_LANGUAGE,ID_GALLERY,ID_REMOVE_BG})control(L"BUTTON",L"",WS_TABSTOP,0,0,1,1,id);
 fpsBox=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,0,0,1,1,ID_FPS);SendMessageW(fpsBox,CB_ADDSTRING,0,(LPARAM)L"30 FPS");SendMessageW(fpsBox,CB_ADDSTRING,0,(LPARAM)L"60 FPS");SendMessageW(fpsBox,CB_SETCURSEL,fps==60?1:0,0);
 autoBox=control(L"BUTTON",L"",BS_AUTOCHECKBOX|WS_TABSTOP,0,0,1,1,ID_AUTO);SendMessageW(autoBox,BM_SETCHECK,policy.automatic?BST_CHECKED:BST_UNCHECKED,0);
 volumeBox=control(TRACKBAR_CLASSW,L"",TBS_AUTOTICKS|WS_TABSTOP,0,0,1,1,ID_VOLUME);SendMessageW(volumeBox,TBM_SETRANGE,TRUE,MAKELONG(0,100));SendMessageW(volumeBox,TBM_SETPOS,TRUE,volume);
 pauseButton=control(L"BUTTON",L"",WS_TABSTOP,0,0,1,1,ID_PAUSE);control(L"BUTTON",L"",WS_TABSTOP,0,0,1,1,ID_ORIGINAL);control(L"BUTTON",L"",WS_TABSTOP,0,0,1,1,ID_STOP);
 statusBox=control(L"STATIC",L"",SS_LEFT,0,0,1,1);detailBox=control(L"STATIC",L"",SS_LEFT,0,0,1,1);
 refreshLanguage();layoutControls();updateUi();
}
void refreshLanguage(){struct Label{int id;const wchar_t*pt,*en;};for(auto t:{Label{ID_CHOOSE,L"Escolher mídia",L"Choose media"},{ID_APPLY,L"Aplicar ao desktop",L"Apply to desktop"},{ID_PREVIEW,L"Testar prévia",L"Test preview"},{ID_ALTERNATIVE,L"Modo alternativo",L"Alternative mode"},{ID_LOGS,L"Diagnóstico",L"Diagnostics"},{ID_ABOUT,L"Sobre",L"About"},{ID_LANGUAGE,L"Idioma: PT / EN",L"Language: EN / PT"},{ID_GALLERY,L"Galeria",L"Gallery"},{ID_REMOVE_BG,L"Tirar fundo",L"Remove background"},{ID_ORIGINAL,L"Voltar ao original",L"Restore original"},{ID_STOP,L"Parar e sair",L"Stop and exit"},{ID_AUTO,L"Pausar em tela cheia",L"Pause for fullscreen"}})SetDlgItemTextW(window,t.id,tr(t.pt,t.en));}
RECT previewBounds(){int x=24,y=352,w=std::max(1,uiWidth-360),h=std::max(180,uiHeight-480);double ratio=16./9;if(checkedSize.width&&checkedSize.height)ratio=double(checkedSize.width)/checkedSize.height;int fw=std::max(1,std::min(w,int(h*ratio))),fh=std::max(1,std::min(h,int(w/ratio)));return {x+(w-fw)/2-scrollX,y+(h-fh)/2-scrollY,x+(w+fw)/2-scrollX,y+(h+fh)/2-scrollY};}
void updatePreviewViewport(){if(interactiveMove||IsIconic(window)||!preview||!IsWindow(video))return;auto v=previewBounds();if(v.right>v.left&&v.bottom>v.top){moveControl(video,v.left,v.top,v.right-v.left,v.bottom-v.top);ShowWindow(video,SW_SHOWNOACTIVATE);}}
void layoutControls(){if(!window||!fileBox||arranging||IsIconic(window))return;RECT r;GetClientRect(window,&r);if(r.right<1||r.bottom<1)return;arranging=true;
 uiWidth=std::max(960L,r.right);uiHeight=std::max(900L,r.bottom);
 SCROLLINFO sx{sizeof(sx),SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL,0,uiWidth-1,(UINT)r.right,scrollX,0};SetScrollInfo(window,SB_HORZ,&sx,TRUE);SCROLLINFO sy{sizeof(sy),SIF_RANGE|SIF_PAGE|SIF_POS|SIF_DISABLENOSCROLL,0,uiHeight-1,(UINT)r.bottom,scrollY,0};SetScrollInfo(window,SB_VERT,&sy,TRUE);scrollX=GetScrollPos(window,SB_HORZ);scrollY=GetScrollPos(window,SB_VERT);
 controlBatch=BeginDeferWindowPos(24);
 int W=uiWidth,H=uiHeight,side=W-310;auto placeH=[&](HWND c,int x,int y,int w,int h){moveControl(c,x-scrollX,y-scrollY,w,h);};auto place=[&](int id,int x,int y,int w,int h){placeH(GetDlgItem(window,id),x,y,w,h);};
 place(ID_GALLERY,24,176,168,40);place(ID_REMOVE_BG,204,176,190,40);place(ID_ABOUT,406,176,130,40);place(ID_LANGUAGE,W-244,176,220,40);
 placeH(fileBox,24,232,W-254,30);place(ID_CHOOSE,W-216,224,192,44);
 place(ID_APPLY,24,284,235,46);place(ID_PREVIEW,271,284,190,46);place(ID_ALTERNATIVE,473,284,210,46);place(ID_LOGS,W-244,284,220,46);
 place(ID_FPS,side,387,150,110);place(ID_AUTO,side,430,280,32);place(ID_VOLUME,side,501,276,32);
 placeH(detailBox,side,565,282,std::max(180,H-700));placeH(statusBox,24,H-112,W-48,44);
 place(ID_PAUSE,24,H-58,180,42);place(ID_ORIGINAL,216,H-58,250,42);place(ID_STOP,W-224,H-58,200,42);
 if(controlBatch)EndDeferWindowPos(controlBatch);controlBatch=nullptr;arranging=false;SetTimer(window,4,90,nullptr);RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
void scrollContent(int bar,int command,int delta=0){SCROLLINFO si{sizeof(si),SIF_ALL};GetScrollInfo(window,bar,&si);int p=si.nPos;switch(command){case SB_LINEUP:p-=40;break;case SB_LINEDOWN:p+=40;break;case SB_PAGEUP:p-=int(si.nPage)-40;break;case SB_PAGEDOWN:p+=int(si.nPage)-40;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:p=si.nTrackPos;break;case SB_TOP:p=si.nMin;break;case SB_BOTTOM:p=si.nMax;break;default:if(delta)p+=delta;else return;}
 si.fMask=SIF_POS;si.nPos=std::clamp(p,0,std::max(0,si.nMax-int(si.nPage)+1));SetScrollInfo(window,bar,&si,TRUE);if(bar==SB_HORZ)scrollX=GetScrollPos(window,bar);else scrollY=GetScrollPos(window,bar);layoutControls();}

void chooseLanguage(){pvlang::english=!pvlang::english;detectHardware();WritePrivateProfileStringW(L"Preferencias",L"Idioma",pvlang::english?L"en":L"pt-BR",iniPath.c_str());refreshLanguage();updateUi();InvalidateRect(window,nullptr,FALSE);}

void writeSelfTest() {
  testPassed = lastError.empty() && pauseMeasured && (player.nativeOutput()?(player.outputReady()&&testPauseDrift<.15&&testProgressed):(player.presents()>20&&pauseDrift<=1&&player.presents()>resumeMark+20));
  std::ofstream f(std::filesystem::path(baseDir + L"\\autoteste.json"));
  f << "{\n  \"ambiente\": \"exe Win32 real; registrar Wine/Windows no "
       "relatorio\",\n"
    << "  \"aprovado\": " << (testPassed ? "true" : "false") << ",\n"
    << "  \"frames\": " << (player.nativeOutput()?"null":std::to_string(player.presents())) << ",\n"
    << "  \"frames_durante_pausa_estavel\": " << (player.nativeOutput()?"null":std::to_string(pauseDrift)) << ",\n"
    << "  \"frames_apos_retomada\": " << (player.nativeOutput()?"null":std::to_string(player.presents()-resumeMark)) << ",\n"
    << "  \"fps_apresentados\": " << (player.nativeOutput()?"null":std::to_string(actualFps)) << ",\n"
    << "  \"fullscreen_detectado\": " << (policy.fullscreen ? "true" : "false")
    << ",\n"
    << "  \"etapa_render\": " << player.stage() << ",\n"
    << "  \"espera_us\": " << player.waitDebug() << ",\n"
    << "  \"callbacks\": " << player.callbacks() << ",\n"
    << "  \"codec\": " << std::quoted(player.text("video-codec")) << ",\n"
    << "  \"vo\": " << std::quoted(player.text("current-vo")) << ",\n"
    << "  \"core_pause\": " << std::quoted(player.text("pause")) << ",\n"
    << "  \"decoder\": " << std::quoted(player.text("hwdec-current")) << ",\n"
    << "  \"posicao_video\": " << player.number("time-pos") << ",\n"
    << "  \"deriva_pausa_segundos\": " << testPauseDrift << ",\n"
    << "  \"progrediu_apos_retomada\": " << (testProgressed?"true":"false") << ",\n"
    << "  \"erro\": " << std::quoted(toUtf8(lastError)) << "\n}\n";
}

LRESULT WindowProcImpl(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (taskbarCreated && m == taskbarCreated) {
    addTray();
    lastRecover = 0;
    return 0;
  }
  switch (m) {
  case WM_CREATE:
    window = h;
    try{playerCatalog.load(catalogPath());}catch(...){recordLog(L"ERRO",L"Galeria inválida; arquivo preservado para diagnóstico.");}
    detectHardware();initControls();queueThumbnails();
    addTray();
    SetTimer(h, 1, 500, nullptr);
    SetTimer(h, 2, 1000, nullptr);
    return 0;
  case WM_GETMINMAXINFO:{auto*p=(MINMAXINFO*)l;MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&mi);int W=mi.rcWork.right-mi.rcWork.left,H=mi.rcWork.bottom-mi.rcWork.top;p->ptMinTrackSize={std::min(780,W),std::min(500,H)};p->ptMaxTrackSize={std::min(1440,W),std::min(1100,H)};p->ptMaxPosition={mi.rcWork.left-mi.rcMonitor.left,mi.rcWork.top-mi.rcMonitor.top};p->ptMaxSize=p->ptMaxTrackSize;return 0;}
  case WM_MOUSEMOVE:{int x=short(LOWORD(l))+scrollX,y=short(HIWORD(l))+scrollY;if(animationsEnabled()&&y<160){int next=std::clamp(x/120-4,-4,4);if(next!=bannerOffset){bannerOffset=next;RECT head{0,-scrollY,uiWidth,160-scrollY};InvalidateRect(h,&head,FALSE);}}return 0;}
  case WM_SIZE:if(w!=SIZE_MINIMIZED)layoutControls();return 0;
  case WM_ENTERSIZEMOVE:interactiveMove=true;KillTimer(h,4);if(preview&&IsWindow(video))ShowWindow(video,SW_HIDE);recordLog(L"UI",L"Begin window move/resize");return 0;
  case WM_EXITSIZEMOVE:interactiveMove=false;layoutControls();recordLog(L"UI",L"End window move/resize");return 0;
  case WM_DPICHANGED:{auto*r=(RECT*)l;if(r)SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);layoutControls();return 0;}
  case WM_VSCROLL:scrollContent(SB_VERT,LOWORD(w));return 0;
  case WM_MOUSEWHEEL:{int steps=short(HIWORD(w))/WHEEL_DELTA;if(GetKeyState(VK_SHIFT)<0)scrollContent(SB_HORZ,99,-steps*90);else scrollContent(SB_VERT,99,-steps*90);return 0;}
  case WM_DRAWITEM:
    drawButton(reinterpret_cast<DRAWITEMSTRUCT *>(l));
    return TRUE;
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC: {
    HDC dc = reinterpret_cast<HDC>(w);
    SetBkColor(dc, RGB(21, 17, 34));
    SetTextColor(dc, RGB(233, 226, 248));
    return reinterpret_cast<LRESULT>(bg);
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT p;
    HDC dc = BeginPaint(h, &p);
    if(mainSurface.size(dc,uiWidth,uiHeight)){paintPanel(mainSurface.dc);mainSurface.present(dc,p.rcPaint,scrollX,scrollY);}
    EndPaint(h, &p);
    return 0;
  }
  case WM_COPYDATA: {
    auto *d = reinterpret_cast<COPYDATASTRUCT *>(l);
    if (!d || (d->dwData != 2000 && d->dwData != 2001) || !d->lpData || d->cbData < 2 || d->cbData > 65536 || d->cbData % 2) return FALSE;
    const wchar_t *p = static_cast<const wchar_t *>(d->lpData);
    if (p[d->cbData / 2 - 1] != 0) return FALSE;
    std::wstring file(p, d->cbData / 2 - 1);
    if (file.size() < 3 || file[1] != L':' || file.find(L"://") != std::wstring::npos || GetFileAttributesW(file.c_str()) == INVALID_FILE_ATTRIBUTES || (GetFileAttributesW(file.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
    selected=file;requestVideo(Intent::Desktop);
    return TRUE;
  }
  case WM_APP+94:{HWND child=(HWND)w;if(!IsWindow(child)||GetParent(child)!=h)return 0;RECT c,r;GetWindowRect(child,&c);MapWindowPoints(nullptr,h,(POINT*)&c,2);GetClientRect(h,&r);int x=scrollX,y=scrollY;if(c.left<12)x+=c.left-12;else if(c.right>r.right-12)x+=c.right-r.right+12;if(c.top<12)y+=c.top-12;else if(c.bottom>r.bottom-12)y+=c.bottom-r.bottom+12;scrollX=std::clamp(x,0,std::max(0,uiWidth-int(r.right)));scrollY=std::clamp(y,0,std::max(0,uiHeight-int(r.bottom)));if(x!=scrollX||y!=scrollY||c.left<12||c.right>r.right-12||c.top<12||c.bottom>r.bottom-12)layoutControls();return 0;}
  case WM_APP + 93:
    return static_cast<LRESULT>(player.number(w ? "height" : "width"));
  case WM_APP + 92:
    if (playing) { policy.manual = w != 0; reconcilePause(); }
    return 0;
  case WM_COMMAND: {
    int id = LOWORD(w);
    if (HIWORD(w) == BN_CLICKED) {
      wchar_t caption[128]{};
      GetDlgItemTextW(h, id, caption, 128);
      recordLog(L"AÇÃO",
                std::wstring(caption) + L" (" + std::to_wstring(id) + L")");
    }
    switch (id) {
    case ID_CHOOSE:
      choose();
      break;
    case ID_APPLY:
      apply(false);
      break;
    case ID_PREVIEW:
      apply(true);
      break;
    case ID_PAUSE:
      if (playing) {
        policy.manual = !policy.manual;
        reconcilePause();
      }
      break;
    case ID_ALTERNATIVE:
      requestVideo(Intent::Alternative);
      break;
    case ID_SAVE_CLOSE:save();PostMessageW(h,WM_CLOSE,0,0);break;
    case ID_GALLERY:openGallery();break;
    case ID_REMOVE_BG:startRemoval();break;
    case ID_STOP:
      PostMessageW(h,WM_CLOSE,0,0);
      break;
    case ID_ORIGINAL:
      media.stop();
      converting = false;
      removeVideo();
      break;
    case ID_SHOW:
      show();
      break;
    case ID_EXIT:
      PostMessageW(h,WM_CLOSE,0,0);
      break;
    case ID_AUTO:
      policy.automatic =
          SendMessageW(autoBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
      save();
      realPlaybackActive();
      checkEnvironment();
      break;
    case ID_FPS:
      if (HIWORD(w) == CBN_SELCHANGE) {
        fps = SendMessageW(fpsBox, CB_GETCURSEL, 0, 0) == 1 ? 60 : 30;
        player.fps(fps);
        save();
      }
      break;
    case ID_LANGUAGE:
      chooseLanguage();
      break;
    case ID_FILES:
      ShellExecuteW(h, L"open", baseDir.c_str(), nullptr, nullptr, SW_SHOW);
      break;
    case ID_LOGS:
      showLogs();
      break;
    case ID_ABOUT:
      friendlyNotice(pvlang::english?L"Papel Vivo 2.0.1\nLocal videos and images. libmpv playback, offline background removal and a reversible gallery.\nOpen the PT/EN manuals and third-party licenses in the application folder.":L"Papel Vivo 2.0.1\nVídeos e imagens locais. Reprodução libmpv, tira-fundo offline e galeria com desfazer.\nConsulte os manuais PT/EN e as licenças na pasta do aplicativo.",pvlang::english?L"About Papel Vivo":L"Sobre o Papel Vivo");
      break;
    }
    return 0;
  }
  case WM_HSCROLL:
    if(!l){scrollContent(SB_HORZ,LOWORD(w));return 0;}
    if (reinterpret_cast<HWND>(l) == volumeBox) {
      volume = static_cast<int>(SendMessageW(volumeBox, TBM_GETPOS, 0, 0));
      player.volume(volume);
      save();
    }
    return 0;
  case WM_TRAY:
    if (LOWORD(l) == WM_CONTEXTMENU || LOWORD(l) == WM_RBUTTONUP)
      menu();
    else if (LOWORD(l) == NIN_SELECT || LOWORD(l) == WM_LBUTTONDBLCLK)
      show();
    return 0;
  case WM_CHECK:
    checkEnvironment();
    return 0;
  case WM_TIMER:
    if (w == 1) {
      pollRemoval();pollThumbnails();
      realPlaybackActive();
      checkEnvironment();
      if (playing && alternative)
        sinkAlternative(video);
      if (playing && !preview && !alternative &&
          !attachedWallpaper(video, desktopTarget) &&
          !positionWallpaper(video, desktopTarget))
        applicationPending = true; // nunca mantém sucesso se perdeu o encaixe
      if (playing && applicationPending) {
        const bool attached = alternative
                                  ? coversPrimary(video)
                                  : attachedWallpaper(video, desktopTarget);
        if (!attached) {
          if (!alternative) {
            useOverlay(policy.manual);
          } else {
            removeVideo();
            error(L"O modo alternativo não pôde ser exibido. Seu original foi "
                  L"preservado.",
                  false);
          }
        } else if (fileLoaded && (player.nativeOutput()?player.outputReady():player.presents()>loadedFrameMark)) {
          applicationPending = false;
          reconcilePause();
        } else if (GetTickCount64() - applicationStarted > 30000) {
          removeVideo();
          error(L"O vídeo não começou a renderizar. Não foi confirmado como "
                L"aplicado.",
                false);
        }
      }
      if (playing && !preview && !alternative &&
          (!IsWindow(video) || !targetStillValid(desktopTarget)) &&
          GetTickCount64() - lastRecover > 5000) {
        lastRecover = GetTickCount64();
        if (findDesktopTarget()) {
          const bool manual = policy.manual;
          startPlayback(false);
          policy.manual = manual;
          reconcilePause();
        } else {
          removeVideo();
          error(L"O Explorer mudou. O vídeo foi parado com segurança; aplique "
                L"novamente quando o desktop estiver disponível.",
                false);
        }
      }
    }
    if(w==4){KillTimer(h,4);updatePreviewViewport();return 0;}
    if(w==3){InvalidateRect(window,nullptr,FALSE);return 0;}
    if (w == 2) {
      telemetry();
      if (selftest && fileLoaded) {
        auto elapsed = GetTickCount64() - testStart;
        if(!testWarmed){
          if(player.outputReady()&&player.number("time-pos")>.05){testWarmed=true;testStart=GetTickCount64();}
          else if(elapsed>60000){writeSelfTest();quitting=true;DestroyWindow(h);}
          return 0;
        }
        if (elapsed > 3000 && !testPaused) {
          policy.manual = true;
          reconcilePause();
          testPaused = true;
          pausedFrames = player.presents();
        }
        if (elapsed > 4000 && !pauseMeasured) {
          pauseStable = player.presents();testPausePos=player.number("time-pos");
          pauseMeasured = true;
        }
        if (elapsed > 5000 && !testResumed) {
          pauseDrift = player.presents() - pauseStable;testPauseDrift=std::abs(player.number("time-pos")-testPausePos);testResumePos=player.number("time-pos");
          resumeMark = player.presents();
          policy.manual = false;
          reconcilePause();
          testResumed = true;
        }
        if(testResumed&&std::abs(player.number("time-pos")-testResumePos)>.15)testProgressed=true;
        if (elapsed > 10000) {
          writeSelfTest();
          quitting = true;
          DestroyWindow(h);
        }
      }
    }
    return 0;
  case WM_PV_MEDIA:
    if(reinterpret_cast<MediaJob*>(l)==&thumbnailJob){thumbnailFinished();return 0;}
    if (media.busy() && w == media.token())
      mediaFinished();
    return 0;
  case WM_PV_MPV: {
    bool failure = false;
    std::wstring why;
    player.events([&](const mpv_event &e) {
      if (e.event_id == MPV_EVENT_LOG_MESSAGE) {
        auto *log = static_cast<mpv_event_log_message *>(e.data);
        if (log && log->log_level <= MPV_LOG_LEVEL_WARN)
          recordLog(fromUtf8(log->level),
                    fromUtf8(std::string(log->prefix) + ": " + log->text));
      }

      if (e.event_id == MPV_EVENT_FILE_LOADED) {
        if (player.text("video-codec").empty()) {
          failure = true;
          why = L"Este arquivo não contém um vídeo compatível.";
        } else {
          lastError.clear();
          loadedFrameMark = player.presents();
          fileLoaded = true;if(selftest)testStart=GetTickCount64();
        }
      }
      if(e.event_id==MPV_EVENT_SHUTDOWN){failure=true;why=L"O núcleo de reprodução foi encerrado. Você pode aplicar novamente.";}
      if (e.event_id == MPV_EVENT_END_FILE) {
        auto *end = static_cast<mpv_event_end_file *>(e.data);
        if (end && end->reason == MPV_END_FILE_REASON_ERROR) {
          failure = true;
          why =
              L"Não foi possível reproduzir este vídeo. Ele pode estar "
              L"corrompido, sem codec suportado ou inacessível. Código mpv: " +
              std::to_wstring(end->error);
        }
      }
      if (e.event_id == MPV_EVENT_COMMAND_REPLY && e.error < 0) {
        failure = true;
        why = L"O mpv recusou abrir o arquivo selecionado. Código mpv: " +
              std::to_wstring(e.error);
      }
    });
    if (failure) {
      removeVideo();
      error(why);
    }
    return 0;
  }
  case WM_PV_RENDER_ERROR:
    removeVideo();
    error(
        L"O dispositivo gráfico foi perdido ou houve erro ao apresentar vídeo. "
        L"O original foi preservado. Atualize o driver e aplique novamente.");
    return 0;
  case WM_DISPLAYCHANGE:
    if (playing && !preview) {
      if (alternative)
        sinkAlternative(video);
      else
        positionWallpaper(video, desktopTarget);
    }
    return 0;
  case WM_WTSSESSION_CHANGE:
    if (w == WTS_SESSION_LOCK)
      policy.locked = true;
    else if (w == WTS_SESSION_UNLOCK)
      policy.locked = false;
    reconcilePause();
    return 0;
  case WM_POWERBROADCAST:
    if (w == PBT_APMSUSPEND)
      policy.suspended = true;
    else if (w == PBT_APMRESUMEAUTOMATIC || w == PBT_APMRESUMESUSPEND)
      policy.suspended = false;
    else if (w == PBT_POWERSETTINGCHANGE) {
      auto *p = reinterpret_cast<POWERBROADCAST_SETTING *>(l);
      if (IsEqualGUID(p->PowerSetting, GUID_CONSOLE_DISPLAY_STATE) &&
          p->DataLength == sizeof(DWORD)) {
        DWORD state = 0;
        CopyMemory(&state, p->Data, sizeof(state));
        policy.displayOff = state == 0;
      }
    }
    reconcilePause();
    return TRUE;
  case WM_QUERYENDSESSION:
    media.stop();
    converting = false;
    removeVideo();
    return TRUE;
  case WM_CLOSE:
    if(playing&&!selftest&&!friendlyConfirm(pvlang::english?L"Papel Vivo will close. Your original wallpaper will return to normal. See you next time!":L"O Papel Vivo vai fechar. Seu wallpaper original voltará ao normal. Até a próxima!",pvlang::english?L"Close Papel Vivo?":L"Fechar Papel Vivo?"))return 0;
    signalShutdown();
    DestroyWindow(h);
    return 0;
  case WM_DESTROY:
    signalShutdown();imageTask.stop();
    thumbnailJob.stop();closeCompanion();quitting = true;
    KillTimer(h, 1);
    KillTimer(h, 2);KillTimer(h,4);
    if (foregroundHook) {
      UnhookWinEvent(foregroundHook);
      foregroundHook = nullptr;
    }
    if (powerNotify)
      UnregisterPowerSettingNotification(powerNotify);
    WTSUnRegisterSessionNotification(h);
    media.stop();
    converting = false;
    removeVideo();
    if (trayAdded)
      Shell_NotifyIconW(NIM_DELETE, &tray);
    window = nullptr;
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}
LRESULT CALLBACK WindowProc(HWND h,UINT m,WPARAM w,LPARAM l) {
  try { return WindowProcImpl(h,m,w,l); }
  catch(const std::exception&e) { recordLog(L"ERROR",L"UI exception: "+fromUtf8(e.what())); if(m==WM_CREATE)return -1; if(m==WM_DESTROY){PostQuitMessage(1);return 0;} friendlyNotice(tr(L"Não foi possível concluir a ação. Consulte Diagnóstico para ver o motivo.",L"The action could not be completed. See Diagnostics for details.")); return 0; }
  catch(...) { recordLog(L"ERROR",L"Unknown UI exception"); if(m==WM_CREATE)return -1; if(m==WM_DESTROY){PostQuitMessage(1);return 0;} return 0; }
}
} // namespace
int WINAPI wWinMain(HINSTANCE in, HINSTANCE, PWSTR, int) {
  instance = in;
  SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                           LOAD_LIBRARY_SEARCH_SYSTEM32);
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  HANDLE singleton =
      CreateMutexW(nullptr, TRUE, L"Local\\PapelVivo.Player.Singleton");
  if (!singleton || GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND other = FindWindowW(L"PapelVivo.Player.Control", nullptr);
    if (other) {
      ShowWindow(other, SW_RESTORE);
      SetForegroundWindow(other);
    }
    if (singleton)
      CloseHandle(singleton);
    return 0;
  }
  SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  INITCOMMONCONTROLSEX cc{sizeof(cc), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
  InitCommonControlsEx(&cc);
  baseDir = moduleDir();
  playing=preview=applicationPending=fileLoaded=false;
  setupPreferences();savedSettings=captureSettings();settingsReady=true;
  setupLog();
  recordLog(L"INÍCIO", L"Papel Vivo 2.0.1 | kz | Win32 x64 | processo " +
                           std::to_wstring(GetCurrentProcessId()));
  if (selected.empty() ||
      GetFileAttributesW(selected.c_str()) == INVALID_FILE_ATTRIBUTES)
    selected = baseDir + L"\\assets\\Brisa-Azul.mp4";
  convertedFolder =
      iniPath.substr(0, iniPath.find_last_of(L"\\")) + L"\\Convertidos";
  int argc = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  bool previewArg = false, desktopArg = false;
  for (int i = 1; i < argc; i++) {
    if (wcscmp(argv[i], L"--sem-aceleracao") == 0)
      diagnosticSoftware = true;
    else if (wcscmp(argv[i], L"--aplicar") == 0)
      desktopArg = true;
    else if (wcscmp(argv[i], L"--visualizar") == 0)
      previewArg = true;
    else if (wcscmp(argv[i], L"--autoteste") == 0) {
      selftest = true;
      previewArg = true;
    } else if (argv[i][0]!=L'-'){selected = argv[i];}
  }
  LocalFree(argv);
  Gdiplus::GdiplusStartupInput gi;
  ULONG_PTR gdip = 0;
  Gdiplus::GdiplusStartup(&gdip, &gi, nullptr);
  initBanner();
  normalFont =
      CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                  DEFAULT_PITCH, L"Segoe UI");
  titleFont =
      CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
  bg = CreateSolidBrush(RGB(21, 17, 34));
  WNDCLASSEXW cls{sizeof(cls)};
  cls.hInstance = in;
  cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  cls.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
  cls.hbrBackground = bg;
  cls.lpfnWndProc = WindowProc;
  cls.lpszClassName = L"PapelVivo.Player.Control";
  RegisterClassExW(&cls);
  cls.lpfnWndProc = VideoProc;
  cls.lpszClassName = L"PapelVivo.Video";
  cls.style = CS_OWNDC;
  RegisterClassExW(&cls);
  // WS_EX_LAYERED é incompatível com CS_OWNDC/CS_CLASSDC. O renderer mantém
  // seu GetDC válido até o fim; a classe layered não usa nenhum desses estilos.
  cls.lpszClassName = L"PapelVivo.VideoLayered";
  cls.style = 0;
  RegisterClassExW(&cls);
  RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
  RECT bounds{0, 0, std::min(1060L,work.right-work.left-40), std::min(740L,work.bottom-work.top-80)};
  AdjustWindowRectEx(&bounds,
                     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                     FALSE, 0);
  HWND h = CreateWindowExW(
      0, L"PapelVivo.Player.Control", L"Papel Vivo 2.0.1 — kz",
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME |
          WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL,
      CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left,
      bounds.bottom - bounds.top, nullptr, nullptr, in, nullptr);
  if (!h) {
    auto why = L"Não foi possível abrir o painel. Código Windows: " +
               std::to_wstring(GetLastError());
    recordLog(L"ERRO", why);
    MessageBoxW(
        nullptr,
        (why + L"\nCtrl+C copia esta mensagem. Logs: " + logPath).c_str(),
        L"Papel Vivo — erro", MB_OK | MB_ICONERROR);
    CloseHandle(singleton);
    return 1;
  }
  taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
  foregroundHook = SetWinEventHook(
      EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, focusEvent, 0,
      0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
  WTSRegisterSessionNotification(h, NOTIFY_FOR_THIS_SESSION);
  powerNotify = RegisterPowerSettingNotification(h, &GUID_CONSOLE_DISPLAY_STATE,
                                                 DEVICE_NOTIFY_WINDOW_HANDLE);
  ShowWindow(h, SW_SHOW);
  UpdateWindow(h);
  if (previewArg && !selected.empty())
    apply(true);
  if (desktopArg && !selected.empty()) apply(false);
  testStart = GetTickCount64();
  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if(msg.message==WM_KEYDOWN&&(GetKeyState(VK_CONTROL)&0x8000)&&(msg.wParam=='Z'||msg.wParam=='Y')){wchar_t cls[64]{};GetClassNameW(GetFocus(),cls,64);if(_wcsicmp(cls,L"EDIT")!=0){if(galleryWindow){if(msg.wParam=='Z')playerCatalog.undo();else playerCatalog.redo();catalogSave();populateGallery();}else undoSettings(msg.wParam=='Y');continue;}}
    if(galleryWindow&&GetAncestor(msg.hwnd,GA_ROOT)==galleryWindow){if(!IsDialogMessageW(galleryWindow,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}continue;}
    if (!IsDialogMessageW(h, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
  DeleteObject(normalFont);
  DeleteObject(titleFont);
  DeleteObject(bg);
  for(auto*im:uiIcons)delete im;
  delete guideArt;delete happyArt;delete banner;
  if (bannerStream)
    bannerStream->Release();
  Gdiplus::GdiplusShutdown(gdip);
  CoUninitialize();
  CloseHandle(singleton);
  return selftest && !testPassed ? 2 : 0;
}
