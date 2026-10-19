#include "media_job.hpp"
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include <mutex>
#include <vector>
#include <objbase.h>

namespace {
struct Handle {
    HANDLE h = nullptr;
    ~Handle() {
        if (h && h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }
    operator HANDLE() const { return h; }
};
struct ProcessResult {
    DWORD code = 1;
    bool cancelled = false;
    std::string text;
};
std::string utf8log(const std::wstring&w){int n=WideCharToMultiByte(CP_UTF8,0,w.data(),int(w.size()),nullptr,0,nullptr,nullptr);std::string s(n,0);WideCharToMultiByte(CP_UTF8,0,w.data(),int(w.size()),s.data(),n,nullptr,nullptr);return s;}
void saveToolLog(const std::wstring&exe,const std::vector<std::wstring>&args,const ProcessResult&r){try{static std::mutex mutex;std::lock_guard<std::mutex> lock(mutex);wchar_t local[32768]{};if(!GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768))return;auto dir=std::filesystem::path(local)/L"PapelVivo"/L"Player"/L"Logs";std::filesystem::create_directories(dir);auto file=dir/L"media-tools.log";std::error_code ec;if(std::filesystem::file_size(file,ec)>2*1024*1024&&!ec){std::filesystem::remove(dir/L"media-tools-anterior.log",ec);std::filesystem::rename(file,dir/L"media-tools-anterior.log",ec);}std::ofstream f(file,std::ios::app|std::ios::binary);f<<"\n[tick="<<GetTickCount64()<<"] "<<utf8log(exe)<<" retornou código "<<r.code<<"; cancelled="<<r.cancelled<<"\n";for(auto&a:args)f<<utf8log(pv::quoteArg(a))<<" ";f<<"\n"<<r.text<<"\n";}catch(...){} }
ProcessResult run(const std::wstring &exe, const std::vector<std::wstring> &args,
                  const std::atomic<bool> &cancel, DWORD timeout) {
    ProcessResult r;
    if (cancel) {
        r.cancelled = true;
        saveToolLog(exe,args,r);return r;
    }
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        r.text = "Ferramenta ausente na instalação. Reinstale o pacote completo.";
        saveToolLog(exe,args,r);return r;
    }
    Handle job{CreateJobObjectW(nullptr, nullptr)};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job.h ||
        !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))){r.code=GetLastError();r.text="CreateJobObject/SetInformationJobObject falhou";saveToolLog(exe,args,r);return r;}
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    Handle reader, writer;
    if (!CreatePipe(&reader.h, &writer.h, &sa, 0)){r.code=GetLastError();r.text="CreatePipe falhou";saveToolLog(exe,args,r);return r;}
    SetHandleInformation(reader, HANDLE_FLAG_INHERIT, 0);
    Handle input{CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (input.h == INVALID_HANDLE_VALUE){r.code=GetLastError();r.text="NUL indisponível";saveToolLog(exe,args,r);return r;}
    std::wstring command = pv::quoteArg(exe);
    for (const auto &arg : args)
        command += L" " + pv::quoteArg(arg);
    STARTUPINFOW si{sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writer;
    si.hStdError = writer;
    si.hStdInput = input;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED | BELOW_NORMAL_PRIORITY_CLASS, nullptr,
                        std::filesystem::path(exe).parent_path().c_str(), &si, &pi)){r.code=GetLastError();r.text="CreateProcessW falhou; Win32="+std::to_string(r.code);saveToolLog(exe,args,r);return r;}
    Handle process{pi.hProcess}, thread{pi.hThread};
    // Se a associação falhar, o filho ainda suspenso é encerrado. Nunca sobra órfão.
    if (!AssignProcessToJobObject(job, process)) {
        TerminateProcess(process, 1);
        WaitForSingleObject(process, INFINITE);
        saveToolLog(exe,args,r);return r;
    }
    CloseHandle(writer.h);
    writer.h = nullptr;
    if (ResumeThread(thread) == DWORD(-1)) {
        TerminateJobObject(job, 1);
        WaitForSingleObject(process, INFINITE);
        saveToolLog(exe,args,r);return r;
    }
    ULONGLONG started = GetTickCount64();
    auto drain = [&] {
        DWORD available = 0;
        for (int chunk = 0;
             chunk < 16 && PeekNamedPipe(reader, nullptr, 0, nullptr, &available, nullptr) &&
             available;
             ++chunk) {
            char data[4096];
            DWORD read = 0;
            if (!ReadFile(reader, data, (available < sizeof(data) ? available : sizeof(data)),
                          &read, nullptr) ||
                !read)
                break;
            r.text.append(data, read);
            if (r.text.size() > 65536)
                r.text.erase(0, r.text.size() - 65536);
        }
    };
    for (;;) {
        drain();
        if (WaitForSingleObject(process, 50) == WAIT_OBJECT_0)
            break;
        if (cancel || GetTickCount64() - started > timeout) {
            r.cancelled = cancel;
            TerminateJobObject(job, 1);
            WaitForSingleObject(process, INFINITE);
            if (!r.cancelled)
                r.text += "\nTempo limite excedido.";
            drain();
            saveToolLog(exe,args,r);return r;
        }
    }
    drain();
    GetExitCodeProcess(process, &r.code);
    saveToolLog(exe,args,r);return r;
}
std::wstring wide(const std::string &s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
ProcessResult probeFile(const std::wstring &tools, const std::wstring &input,
                        const std::atomic<bool> &cancel) {
    return run(tools + L"\\ffprobe.exe",
               {L"-v", L"error", L"-protocol_whitelist", L"file", L"-probesize", L"4M",
                L"-analyzeduration", L"2000000", L"-select_streams", L"v:0", L"-show_entries",
                L"stream=width,height,avg_frame_rate,bit_rate:format=bit_rate", L"-of", L"json", L"-i", input},
               cancel, 15000);
}
std::optional<pv::VideoSize> metadata(const std::string&text){
 try{auto j=nlohmann::json::parse(text);auto v=j.at("streams").at(0);pv::VideoSize s;s.width=v.at("width");s.height=v.at("height");auto rate=v.value("avg_frame_rate","0/1");auto slash=rate.find('/');double den=slash==std::string::npos?1:std::stod(rate.substr(slash+1));s.fps=den>0?std::stod(rate.substr(0,slash))/den:0;auto bits=v.value("bit_rate",j.value("format",nlohmann::json::object()).value("bit_rate","0"));s.bitrate=std::stoull(bits);if(!s.width||!s.height||s.width>16384||s.height>16384||!std::isfinite(s.fps))return {};return s;}catch(...){return {};}
}
std::wstring failure(const ProcessResult &r) {
    return L"Código " + std::to_wstring(r.code) + L". " + wide(r.text.substr(r.text.size()>2400?r.text.size()-2400:0));
}
} // namespace
void MediaJob::probe(HWND owner, const std::wstring &tools, const std::wstring &input) {
    stop();
    cancel_ = false;
    busy_ = true;
    const auto token = ++serial_;
    thread_ = std::thread([=] {
        auto r = probeFile(tools, input, cancel_);
        result_ = {};
        result_.input = input;
        auto size = metadata(r.text);
        if (r.cancelled)
            result_.kind = MediaResult::Cancelled;
        else if (r.code == 0 && size) {
            result_.kind = MediaResult::Probed;
            result_.size = *size;
        } else
            result_.error =
                L"Não foi possível verificar a resolução antes de abrir o vídeo. " + failure(r);
        PostMessageW(owner, WM_PV_MEDIA, token, reinterpret_cast<LPARAM>(this));
    });
}
void MediaJob::convert(HWND owner, const std::wstring &tools, const std::wstring &input,
                       const std::wstring &folder) {
    stop();
    cancel_ = false;
    busy_ = true;
    const auto token = ++serial_;
    thread_ = std::thread([=] {
        result_ = {};
        result_.input = input;result_.attemptedConversion=true;
        auto originalProbe=probeFile(tools,input,cancel_);auto originalSize=metadata(originalProbe.text);if(originalProbe.code==0&&originalSize)result_.size=*originalSize;
        GUID id{};
        wchar_t guid[40]{};
        if (FAILED(CoCreateGuid(&id))) {
            result_.error = L"Não foi possível criar o nome da cópia.";
            PostMessageW(owner, WM_PV_MEDIA, token, reinterpret_cast<LPARAM>(this));
            return;
        }
        StringFromGUID2(id, guid, 40);
        auto final = folder + L"\\copia-leve-" + guid + L".mp4";
        auto partial = final + L".parcial.mp4";
        if (!CreateDirectoryW(folder.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
            result_.error = L"Não foi possível criar a pasta Convertidos.";
            PostMessageW(owner, WM_PV_MEDIA, token, reinterpret_cast<LPARAM>(this));
            return;
        }
        // -n e um GUID impedem sobrescrever qualquer arquivo. Sem shell, sem rede.
        // Dois threads de codec e um de filtro para não saturar o PC fraco.
        ProcessResult r;bool weak=false;MEMORYSTATUSEX memory{sizeof(memory)};SYSTEM_INFO info{};GetSystemInfo(&info);if(GlobalMemoryStatusEx(&memory))weak=memory.ullTotalPhys<6ull*1024*1024*1024||info.dwNumberOfProcessors<=2;
        std::wstring scale=weak?L"scale=w=1280:h=720:force_original_aspect_ratio=decrease:force_divisible_by=2,fps=30":L"scale=w=1920:h=1080:force_original_aspect_ratio=decrease:force_divisible_by=2,fps=30";
        for(auto codec:{L"h264_nvenc",L"h264_mf",L"libx264"}){
          if(cancel_)break;DeleteFileW(partial.c_str());
          std::vector<std::wstring> args{L"-hide_banner",L"-nostdin",L"-n",L"-loglevel",L"error",L"-protocol_whitelist",L"file",L"-threads",L"2",L"-i",input,L"-map",L"0:v:0",L"-map",L"0:a?",L"-filter_threads",L"1",L"-vf",scale,L"-c:v",codec,L"-threads",L"2",L"-pix_fmt",L"yuv420p",L"-maxrate",L"6M",L"-bufsize",L"12M",L"-c:a",L"copy",L"-sn",L"-dn",L"-movflags",L"+faststart"};
          if(std::wstring(codec)==L"libx264")args.insert(args.end(),{L"-preset",L"veryfast",L"-crf",L"23"});
          if(std::wstring(codec)==L"h264_mf")args.insert(args.end(),{L"-hw_encoding",L"1"});
          args.insert(args.end(),{L"-f",L"mp4",partial});r=run(tools+(std::wstring(codec)==L"libx264"?L"\\ffmpeg.exe":L"\\ffmpeg-gpu.exe"),args,cancel_,7200000);if(r.code==0||r.cancelled)break;
        }
        if (r.code == 0 && !r.cancelled && !cancel_) {
            auto verified = probeFile(tools, partial, cancel_);
            auto size = metadata(verified.text);
            if (verified.code == 0 && size && !size->heavy() && !cancel_ &&
                MoveFileExW(partial.c_str(), final.c_str(), MOVEFILE_WRITE_THROUGH)) {
                result_.kind = MediaResult::Converted;
                result_.output = final;
                result_.size = *size;
            } else
                result_.error = L"A cópia não passou na verificação ou não pôde ser salva. O "
                                L"original foi preservado.";
        } else if (cancel_ || r.cancelled)
            result_.kind = MediaResult::Cancelled;
        else
            result_.error = L"A conversão falhou; o original foi preservado. " + failure(r);
        DeleteFileW(partial.c_str());
        PostMessageW(owner, WM_PV_MEDIA, token, reinterpret_cast<LPARAM>(this));
    });
}
MediaResult MediaJob::take() {
    if (thread_.joinable())
        thread_.join();
    busy_ = false;
    return std::move(result_);
}
void MediaJob::stop() {
    ++serial_;
    cancel_ = true;
    if (thread_.joinable())
        thread_.join();
    busy_ = false;
}

void MediaJob::thumbnail(HWND owner,const std::wstring&tools,const std::wstring&input,const std::wstring&output){
 stop();cancel_=false;busy_=true;const auto token=++serial_;
 thread_=std::thread([=]{result_={};result_.input=input;try{std::filesystem::create_directories(std::filesystem::path(output).parent_path());auto r=run(tools+L"\\ffmpeg.exe",{L"-nostdin",L"-v",L"error",L"-protocol_whitelist",L"file",L"-threads",L"1",L"-i",input,L"-frames:v",L"1",L"-vf",L"scale=640:360:force_original_aspect_ratio=decrease",L"-y",output},cancel_,20000);result_.kind=r.cancelled?MediaResult::Cancelled:r.code==0?MediaResult::Thumbnail:MediaResult::Failed;result_.output=output;result_.error=failure(r);}catch(...){result_.error=L"Thumbnail error";}PostMessageW(owner,WM_PV_MEDIA,token,reinterpret_cast<LPARAM>(this));});
}
