#pragma once
#include <algorithm>
#include <cstdint>

namespace pv {
// Política pura, testável sem Explorer, GPU ou Windows.
struct Rect {
    int left, top, right, bottom;
};
inline bool covers(Rect w, Rect m, int tolerance = 2) {
    return w.left <= m.left + tolerance && w.top <= m.top + tolerance &&
           w.right >= m.right - tolerance && w.bottom >= m.bottom - tolerance && w.right > w.left &&
           w.bottom > w.top;
}
struct PausePolicy {
    bool manual = false, locked = false, displayOff = false, suspended = false;
    bool automatic = true, fullscreen = false;
    int64_t clearSince = -1;
    void sample(bool covered, int64_t now) {
        if (covered) {
            fullscreen = true;
            clearSince = -1;
        } else if (fullscreen) {
            if (clearSince < 0)
                clearSince = now;
            if (now - clearSince >= 750)
                fullscreen = false;
        }
    }
    bool paused() const {
        return manual || locked || displayOff || suspended || (automatic && fullscreen);
    }
};
class FrameLimiter {
    int fps_ = 30;
    int64_t next_ = 0;

  public:
    void fps(int n) {
        fps_ = n == 60 ? 60 : 30;
        next_ = 0;
    }
    int fps() const { return fps_; }
    int64_t waitUs(int64_t now) const { return std::max<int64_t>(0, next_ - now); }
    // Agenda a partir da apresentação real: não tenta recuperar atraso em
    // rajadas.
    void presented(int64_t now) { next_ = now + (1000000 + fps_ - 1) / fps_; }
};
} // namespace pv
