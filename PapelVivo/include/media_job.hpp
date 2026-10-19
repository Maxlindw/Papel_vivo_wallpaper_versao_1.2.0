#pragma once
#include "workflow.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <windows.h>
constexpr UINT WM_PV_MEDIA = WM_APP + 23;
struct MediaResult {
    enum Kind { Probed, Converted, Thumbnail, Failed, Cancelled } kind = Failed;
    std::wstring input, output, error;
    pv::VideoSize size;
    bool attemptedConversion=false;
};
// Uma tarefa por vez. O thread nunca toca em HWNDs/Player: só publica resultado.
// Os filhos pertencem a um Job Object privado e morrem em cancelamento/saída.
class MediaJob {
    std::thread thread_;
    std::atomic<bool> cancel_{false};
    ULONG_PTR serial_ = 0;
    bool busy_ = false; // acessado somente pela thread Win32
    MediaResult result_;

  public:
    ~MediaJob() { stop(); }
    ULONG_PTR token() const { return serial_; }
    bool busy() const { return busy_; }
    void probe(HWND owner, const std::wstring &tools, const std::wstring &input);
    void convert(HWND owner, const std::wstring &tools, const std::wstring &input,
                 const std::wstring &folder);
    void thumbnail(HWND owner,const std::wstring &tools,const std::wstring &input,const std::wstring &output);
    MediaResult take(); // mensagem enviada depois de terminar; join sincroniza resultado
    void stop();
};
