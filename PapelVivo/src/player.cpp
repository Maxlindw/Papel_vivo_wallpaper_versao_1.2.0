#include "player.hpp"
#include <algorithm>
#include <chrono>

namespace {
std::wstring wide(const char *s) {
    if (!s)
        return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring v(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, v.data(), n);
    if (n)
        v.pop_back();
    return v;
}
int64_t clockUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
} // namespace
bool Player::start(HWND notify, HWND target, const std::wstring &dll, int fps, int volume,
                   std::wstring &error, bool software) {
    stop();
    notify_ = notify;
    target_ = target;
    cap_ = fps;
    native_ = !software;
    presents_ = 0;
    callbacks_ = 0;
    RECT r{};
    GetClientRect(target_, &r);
    width_ = std::max(1L, r.right);
    height_ = std::max(1L, r.bottom);
    if (!api_.module && !api_.load(dll, error))
        return false;
    core_ = api_.mpv_create();
    if (!core_) {
        error = L"Não foi possível iniciar a libmpv.";
        return false;
    }
    auto option = [&](const char *key, const char *value) {
        int e = api_.mpv_set_option_string(core_, key, value);
        if (e < 0) {
            error =
                L"Opção mpv incompatível: " + wide(key) + L" — " + wide(api_.mpv_error_string(e));
            return false;
        }
        return true;
    };
    // Sem configuração global, scripts, rede, cache grande ou interpolação.
    const char *options[][2] = {{"config", "no"},
                                {"load-scripts", "no"},
                                {"terminal", "no"},
                                {"input-default-bindings", "no"},
                                {"input-vo-keyboard", "no"},
                                {"osc", "no"},
                                {"sid", "no"},
                                {"sub-auto", "no"},
                                {"vo", software ? "libmpv" : "gpu"},
                                {"hwdec", software ? "no" : "d3d11va"},
                                {"hwdec-extra-frames", "2"},
                                {"loop-file", "inf"},
                                {"image-display-duration","inf"},
                                {"keep-open", "yes"},
                                {"cache", "no"},
                                {"demuxer-max-bytes", "8MiB"},
                                {"demuxer-max-back-bytes", "0"},
                                {"vd-lavc-threads", "2"},
                                {"vd-lavc-dr", "no"},
                                {"interpolation", "no"},
                                {"scale", "bilinear"},
                                {"dscale", "bilinear"},
                                {"cscale", "bilinear"},
                                {"deband", "no"},
                                {"dither-depth", "no"},
                                {"panscan", "1.0"},
                                {"keepaspect", "yes"},
                                {"video-unscaled", "no"},
                                {"stop-screensaver", "no"},
                                {"video-timing-offset", "0"},
                                {"correct-downscaling", "no"},
                                {"access-references", "no"},
                                {"demuxer-lavf-o", "protocol_whitelist=file"}};
    for (auto &o : options)
        if (!option(o[0], o[1])) {
            stop();
            return false;
        }
    if (native_) {
        auto wid=std::to_string(reinterpret_cast<uintptr_t>(target_));
        if(!option("gpu-context","d3d11")||!option("wid",wid.c_str())||!option("force-window","yes")||!option("d3d11-sync-interval","1")) {stop();return false;}
    }
    if (!option("aid", volume > 0 ? "auto" : "no")) {
        stop();
        return false;
    }
    if (software)
        option("hwdec", "no");
    if (api_.mpv_initialize(core_) < 0) {
        error = L"Falha ao inicializar mpv.";
        stop();
        return false;
    }
    api_.mpv_request_log_messages(core_, "info");
    if(native_) {api_.mpv_set_wakeup_callback(core_,wakeCore,this);this->volume(volume);this->fps(fps);return true;}

    stop_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    update_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ready_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stop_ || !update_ || !ready_) {
        error = L"Não foi possível criar eventos de renderização.";
        stop();
        return false;
    }
    initialization_ = MPV_ERROR_GENERIC;
    api_.mpv_set_wakeup_callback(core_, wakeCore, this);
    thread_ = std::thread(&Player::run, this);
    WaitForSingleObject(ready_, INFINITE);
    if (initialization_ < 0) {
        error = L"Não foi possível criar o renderer OpenGL: " +
                wide(api_.mpv_error_string(initialization_)) +
                L". Use a DLL e um driver compatíveis.";
        stop();
        return false;
    }
    this->volume(volume);
    return true;
}
void Player::run() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    // Inicialização, uso e liberação do renderer no MESMO thread. Evita
    // dependência de caches/TLS do compilador de shaders entre threads.
    int advanced = 0;

    glDC_ = GetDC(target_);
    PIXELFORMATDESCRIPTOR fmt{};
    fmt.nSize = sizeof(fmt);
    fmt.nVersion = 1;
    fmt.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    fmt.iPixelType = PFD_TYPE_RGBA;
    fmt.cColorBits = 32;
    fmt.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(glDC_, &fmt);
    if (!pf || !SetPixelFormat(glDC_, pf, &fmt) ||
        (glContext_ = wglCreateContext(glDC_)) == nullptr || !wglMakeCurrent(glDC_, glContext_)) {
        initialization_ = MPV_ERROR_UNSUPPORTED;
        SetEvent(ready_);
        return;
    }
    using SwapInterval = BOOL(WINAPI *)(int);
    auto proc = glProc(nullptr, "wglSwapIntervalEXT");
    SwapInterval interval = nullptr;
    static_assert(sizeof(interval) == sizeof(proc));
    std::memcpy(&interval, &proc, sizeof(proc));
    if (interval)
        interval(0);
    mpv_opengl_init_params init{glProc, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advanced},
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &init},
        {MPV_RENDER_PARAM_INVALID, nullptr}};
    initialization_ = api_.mpv_render_context_create(&render_, core_, params);

    if (initialization_ < 0) {
        SetEvent(ready_);
        return;
    }
    api_.mpv_render_context_set_update_callback(render_, wakeRender, this);
    SetEvent(ready_);
    HANDLE handles[] = {stop_, update_};
    pv::FrameLimiter limiter;
    limiter.fps(cap_);
    unsigned w = width_, h = height_;
    for (;;) {
        stage_ = 1;
        if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) == WAIT_OBJECT_0)
            break;
        stage_ = 2;
        auto flags = api_.mpv_render_context_update(render_);
        bool resized = redraw_.exchange(false);
        if (!(flags & MPV_RENDER_UPDATE_FRAME) && !resized)
            continue;
        if (cap_ != limiter.fps())
            limiter.fps(cap_);
        // Espera bloqueante e interrompível. Sem busy-wait, Sleep(0) ou filtro fps
        // de conversão temporal. WGL pode usar hwdec copy-back; veja README.
        stage_ = 3;
        auto wait = limiter.waitUs(clockUs());
        waitDebug_ = wait;
        if (wait > 0 &&
            WaitForSingleObject(stop_, static_cast<DWORD>((wait + 999) / 1000)) == WAIT_OBJECT_0)
            break;

        w = width_;
        h = height_;
        mpv_opengl_fbo fbo{0, static_cast<int>(w), static_cast<int>(h), 0};
        int flip = 1, timing = 0;
        mpv_render_param p[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                                {MPV_RENDER_PARAM_FLIP_Y, &flip},
                                {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &timing},
                                {MPV_RENDER_PARAM_INVALID, nullptr}};
        stage_ = 5;
        int e = api_.mpv_render_context_render(render_, p);
        if (e < 0 || !SwapBuffers(glDC_)) {
            PostMessageW(notify_, WM_PV_RENDER_ERROR, 0, 0);
            break;
        }

        stage_ = 7;
        api_.mpv_render_context_report_swap(render_);
        presents_++;
        limiter.presented(clockUs());
    }
    // Libera o render context ANTES do core. Não chama client API neste thread.
    api_.mpv_render_context_set_update_callback(render_, nullptr, nullptr);
    api_.mpv_render_context_free(render_);
    render_ = nullptr;
    if (glContext_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (glDC_) {
        ReleaseDC(target_, glDC_);
        glDC_ = nullptr;
    }
}
void Player::stop() {
    if (core_)
        api_.mpv_set_wakeup_callback(core_, nullptr, nullptr);
    if (stop_)
        SetEvent(stop_);
    if (thread_.joinable())
        thread_.join();
    if (render_) {
        api_.mpv_render_context_free(render_);
        render_ = nullptr;
    }
    if (glContext_) {
        wglDeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (glDC_) {
        ReleaseDC(target_, glDC_);
        glDC_ = nullptr;
    }
    if (core_) {
        auto old=core_;core_=nullptr;
        // Native Win32 VO destroys child HWNDs on its own thread. Keep the
        // owner thread pumping so cross-thread window messages cannot deadlock.
        if(native_){HANDLE finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
          std::thread cleanup([&]{api_.mpv_terminate_destroy(old);SetEvent(finished);});
          while(MsgWaitForMultipleObjects(1,&finished,FALSE,INFINITE,QS_SENDMESSAGE)!=WAIT_OBJECT_0){MSG msg;PeekMessageW(&msg,nullptr,0,0,PM_NOREMOVE);}
          cleanup.join();CloseHandle(finished);
        }else api_.mpv_terminate_destroy(old);
    }
    if (update_)
        CloseHandle(update_);
    if (stop_)
        CloseHandle(stop_);
    if (ready_)
        CloseHandle(ready_);
    ready_ = nullptr;
    update_ = stop_ = nullptr;
    api_.unload();native_=false;target_=nullptr;notify_=nullptr;
}
void Player::load(const std::string &f) {
    if (core_) {
        const char *args[] = {"loadfile", f.c_str(), "replace", nullptr};
        api_.mpv_command_async(core_, 1, args);
    }
}
void Player::pause(bool b) {
    if (core_) {
        int v = b;
        api_.mpv_set_property(core_, "pause", MPV_FORMAT_FLAG, &v);
    }
}
void Player::volume(int v) {
    if (core_) {
        v = std::clamp(v, 0, 100);
        api_.mpv_set_property_string(core_, "aid", v ? "auto" : "no");
        double d = v;
        api_.mpv_set_property(core_, "volume", MPV_FORMAT_DOUBLE, &d);
    }
}
std::string Player::text(const char *n) {
    if (!core_)
        return {};
    char *p = api_.mpv_get_property_string(core_, n);
    std::string s = p ? p : "";
    api_.mpv_free(p);
    return s;
}
double Player::number(const char *n) {
    double d = 0;
    if (core_)
        api_.mpv_get_property(core_, n, MPV_FORMAT_DOUBLE, &d);
    return d;
}

void *Player::glProc(void *, const char *name) {
    PROC proc = wglGetProcAddress(name);
    auto value = reinterpret_cast<uintptr_t>(proc);
    if (!proc || value <= 3 || value == static_cast<uintptr_t>(-1))
        proc = GetProcAddress(GetModuleHandleW(L"opengl32.dll"), name);
    void *result = nullptr;
    static_assert(sizeof(result) == sizeof(proc));
    std::memcpy(&result, &proc, sizeof(result));
    return result;
}

void Player::fps(int value){cap_=value==60?60:30;if(core_&&native_){HDC d=GetDC(target_);int hz=GetDeviceCaps(d,VREFRESH);ReleaseDC(target_,d);int interval=std::clamp((std::max(1,hz)+cap_-1)/cap_,1,4);api_.mpv_set_property_string(core_,"options/d3d11-sync-interval",std::to_string(interval).c_str());}}
