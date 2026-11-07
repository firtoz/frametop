// Crisp resize feedback only: hit geometry and corner selection remain independent.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace resizecorner {
struct Style {
    bool sharp = true, native = false;
    uint32_t color = 0xf6f9ff, hoverColor = 0x66ddff;
    double opacity = 0.9, lengthCm = 4, widthMm = 3, gapCm = 2;
    double targetCm = 12, reachCm = 20, roundness = 100;
    bool preview = false;
    bool always[4] = {};  // top left, top right, bottom left, bottom right
};

inline double Animate(double value, bool ready, double dt) {
    const double step = std::clamp(dt, 0.0, 0.1) / 0.18;
    return ready ? std::min(1.0, value + step) : std::max(0.0, value - step);
}
inline double Ease(double value) { return value * value * (3 - 2 * value); }
inline double Strength(bool visible, bool always, double proximity, bool ready) {
    return !visible ? 0 : ready ? 1 : std::max(std::clamp(proximity, 0.0, 1.0), always ? 0.65 : 0.0);
}

// Same square as the invisible resize overlay; diagnostic only, never interactive.
inline std::vector<uint8_t> TargetTexture(int n) {
    std::vector<uint8_t> px(size_t(n) * n * 4, 0);
    for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
        auto *p = &px[(size_t(y) * n + x) * 4];
        p[0] = 102, p[1] = 221, p[2] = 255;
        p[3] = x < 1 || y < 1 || x >= n - 1 || y >= n - 1 ? 180 : 24;
    }
    return px;
}

inline double Canvas(const Style &style) {
    return 2 * (std::max(style.lengthCm * (style.native ? 1.12 : 1), style.gapCm + (style.native ? 0.3 : 0)) / 100
                + style.widthMm * 1.6 / 1000 + 0.004);
}

// A round-ended L, with a quarter-circle joining the arms outside the panel.
// Signed coordinates point out of the corner, so all four orientations share it.
inline std::vector<uint8_t> Texture(int n, double size, int sx, int sy, const Style &style, double hover = 0) {
    std::vector<uint8_t> px(size_t(n) * n * 4, 0);
    hover = std::clamp(hover, 0.0, 1.0);
    const double arm = style.lengthCm / 100 * (1 + (style.native ? 0.12 : 0) * hover);
    const double line = style.widthMm / 1000 * (1 + (style.native ? 0.2 : 0.6) * hover);
    const double gap = style.gapCm / 100 + line / 2 + (style.native ? 0.003 * hover : 0), pixel = size / n;
    const double radius = gap * std::clamp(style.roundness, 0.0, 100.0) / 100, centre = gap - radius;
    const uint32_t from = style.native ? 0xaaaaaa : style.color, to = style.native ? 0xfafafa : style.hoverColor;
    auto channel = [&](int shift) { return uint8_t(std::lround(((from >> shift) & 255) * (1 - hover) + ((to >> shift) & 255) * hover)); };
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const double X = sx * (x + 0.5 - n / 2.0) * pixel;
            const double Y = sy * (n / 2.0 - y - 0.5) * pixel;
            const double horizontal = std::hypot(X - std::clamp(X, -arm, centre), Y - gap);
            const double vertical = std::hypot(X - gap, Y - std::clamp(Y, -arm, centre));
            double d = std::min(horizontal, vertical);
            if (X >= centre && Y >= centre) d = std::min(d, std::fabs(std::hypot(X - centre, Y - centre) - radius));
            // One-pixel coverage AA, not a diffuse halo. Keep the panel completely clear.
            const double a = X < 0 && Y < 0 ? 0 : std::clamp((line / 2 - d) / pixel + 0.5, 0.0, 1.0);
            auto *p = &px[(size_t(y) * n + x) * 4];
            p[0] = channel(16), p[1] = channel(8), p[2] = channel(0);
            p[3] = uint8_t(std::lround(a * 255));
        }
    return px;
}
}  // namespace resizecorner
