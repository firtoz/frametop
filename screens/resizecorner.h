// Crisp resize feedback only: hit geometry and corner selection remain independent.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace resizecorner {
struct Style {
    bool sharp = true;
    uint32_t color = 0xf6f9ff;
    double opacity = 0.9, lengthCm = 4, widthMm = 2;
};

// A round-ended L, with a quarter-circle joining the arms outside the panel.
// Signed coordinates point out of the corner, so all four orientations share it.
inline std::vector<uint8_t> Texture(int n, double size, int sx, int sy, const Style &style) {
    std::vector<uint8_t> px(size_t(n) * n * 4, 0);
    const double arm = style.lengthCm / 100, line = style.widthMm / 1000;
    const double gap = line / 2 + 0.002, pixel = size / n;
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const double X = sx * (x + 0.5 - n / 2.0) * pixel;
            const double Y = sy * (n / 2.0 - y - 0.5) * pixel;
            const double horizontal = std::hypot(X - std::clamp(X, -arm, 0.0), Y - gap);
            const double vertical = std::hypot(X - gap, Y - std::clamp(Y, -arm, 0.0));
            double d = std::min(horizontal, vertical);
            if (X >= 0 && Y >= 0) d = std::min(d, std::fabs(std::hypot(X, Y) - gap));
            // One-pixel coverage AA, not a diffuse halo. Keep the panel completely clear.
            const double a = X < 0 && Y < 0 ? 0 : std::clamp((line / 2 - d) / pixel + 0.5, 0.0, 1.0);
            auto *p = &px[(size_t(y) * n + x) * 4];
            p[0] = uint8_t(style.color >> 16), p[1] = uint8_t(style.color >> 8), p[2] = uint8_t(style.color);
            p[3] = uint8_t(std::lround(a * 255));
        }
    return px;
}
}  // namespace resizecorner
