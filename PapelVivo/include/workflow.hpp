#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <optional>
#include <string>

namespace pv {
struct VideoSize {
    unsigned width = 0, height = 0;
    double fps=0;unsigned long long bitrate=0;
    bool retryAllowed(uint64_t totalBytes,uint64_t availableBytes) const {
      if(!width||!height||!std::isfinite(fps)||fps<=0||width>=3840||height>=2160||uint64_t(width)*height>5500000||fps>60.01||bitrate>30000000||availableBytes<512ull*1024*1024)return false;
      if(totalBytes<4ull*1024*1024*1024&&(width>1920||height>1080||fps>30.01||bitrate>12000000))return false;return true;
    }
    bool heavy() const { return width > 1920 || height > 1080 || fps>60.01 || bitrate>20000000; }
};
inline bool needsOptimization(const VideoSize&s,unsigned cores,uint64_t totalRam){bool weak=cores<=2||totalRam<6ull*1024*1024*1024;return s.heavy()||(weak&&(s.width>1280||s.height>720||s.fps>30.01||s.bitrate>6000000));}
inline std::optional<VideoSize> dimensions(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.pop_back();
    VideoSize s;
    const char *begin = text.data(), *end = begin + text.size();
    auto w = std::from_chars(begin, end, s.width);
    if (w.ec != std::errc{} || w.ptr == end || *w.ptr != 'x')
        return {};
    auto h = std::from_chars(w.ptr + 1, end, s.height);
    if (h.ec != std::errc{} || h.ptr != end || !s.width || !s.height || s.width > 100000 ||
        s.height > 100000)
        return {};
    return s;
}
// Escape de argv do CreateProcess/CRT, NÃO de um shell. Aspas e barras finais
// são preservadas mesmo em caminhos com espaços. Nenhum cmd.exe é usado.
inline std::wstring quoteArg(const std::wstring &value) {
    std::wstring result = L"\"";
    unsigned slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') {
            ++slashes;
            continue;
        }
        if (c == L'\"')
            result.append(slashes * 2 + 1, L'\\');
        else
            result.append(slashes, L'\\');
        slashes = 0;
        result += c;
    }
    result.append(slashes * 2, L'\\');
    return result + L'\"';
}
inline bool usesRaisedDesktop(uintptr_t extendedStyle) {
    return (extendedStyle & 0x00200000u) != 0;
}
} // namespace pv
