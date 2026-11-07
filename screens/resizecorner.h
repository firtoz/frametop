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
    double opacity = 0.9, lengthCm = 4, widthMm = 2, gapCm = 2;
    double targetCm = 12, reachCm = 20, roundness = 100;
};

inline double Canvas(const Style &style) {
    return 2 * (std::max(style.lengthCm * (style.native ? 1.12 : 1), style.gapCm + (style.native ? 0.3 : 0)) / 100
                + style.widthMm * 1.6 / 1000 + 0.004);
}

// A round-ended L, with a quarter-circle joining the arms outside the panel.
// Signed coordinates point out of the corner, so all four orientations share it.
inline std::vector<uint8_t> Texture(int n, double size, int sx, int sy, const Style &style, bool ready = false) {
    std::vector<uint8_t> px(size_t(n) * n * 4, 0);
    const double arm = style.lengthCm / 100 * (ready && style.native ? 1.12 : 1);
    const double line = style.widthMm / 1000 * (ready ? (style.native ? 1.2 : 1.6) : 1);
    const double gap = style.gapCm / 100 + line / 2 + (ready && style.native ? 0.003 : 0), pixel = size / n;
    const double radius = gap * std::clamp(style.roundness, 0.0, 100.0) / 100, centre = gap - radius;
    const uint32_t color = style.native ? (ready ? 0xfafafa : 0xaaaaaa) : ready ? style.hoverColor : style.color;
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
            p[0] = uint8_t(color >> 16), p[1] = uint8_t(color >> 8), p[2] = uint8_t(color);
            p[3] = uint8_t(std::lround(a * 255));
        }
    return px;
}
}  // namespace resizecorner
