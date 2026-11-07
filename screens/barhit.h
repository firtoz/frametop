// Drag-bar target geometry and its optional preview, with no OpenVR dependency.
// A padded canvas keeps the 256x24 pill the same physical size on a larger overlay.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace barhit {
constexpr double kDefaultScale = 1, kMaxScale = 15;
constexpr double kAspect = 24.0 / 256;

inline double Scale(double scale) {
    return std::isfinite(scale) ? std::clamp(scale, kDefaultScale, kMaxScale) : kDefaultScale;
}
inline double Height(double width, double scale) { return width * kAspect * Scale(scale); }
inline double Width(double width, double scale) { return width * Scale(scale); }
inline double Below(double width, double button, double scale) {
    return width * 0.06 + std::max(Height(width, scale) / 2, button / 2);
}

struct Style {
    bool enabled = false, always = false;
    uint32_t color = 0x66bbff, outlineColor = 0x99ddff;
    double opacity = 0.12, outlineOpacity = 0.7, outlineWidth = 1;
};

inline bool Color(const char *text, uint32_t *out) {
    if (std::strlen(text) != 7 || text[0] != '#') return false;
    uint32_t n = 0;
    for (int i = 1; i < 7; ++i) {
        const char c = text[i];
        const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                      c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (d < 0) return false;
        n = n * 16 + unsigned(d);
    }
    *out = n;
    return true;
}

struct Image { int width = 0, height = 0; std::vector<uint8_t> pixels; };

// Straight-alpha source-over, so the visible pill stays intact over a translucent zone.
inline void Over(uint8_t *dst, const uint8_t *src) {
    const double a = src[3] / 255.0, b = dst[3] / 255.0 * (1 - a), total = a + b;
    if (total <= 0) return;
    for (int c = 0; c < 3; ++c) dst[c] = uint8_t(std::lround((src[c] * a + dst[c] * b) / total));
    dst[3] = uint8_t(std::lround(total * 255));
}

inline Image Render(double widthScale, double heightScale, const Style &style, const std::vector<uint8_t> &pill) {
    Image im;
    im.width = int(std::lround(256 * Scale(widthScale)));
    im.height = int(std::lround(24 * Scale(heightScale)));
    im.pixels.resize(size_t(im.width) * im.height * 4, 0);
    if (style.enabled) {
        const int line = int(std::lround(std::clamp(style.outlineWidth, 0.0, 8.0)));
        for (int y = 0; y < im.height; ++y)
            for (int x = 0; x < im.width; ++x) {
                const bool edge = x < line || y < line || x >= im.width - line || y >= im.height - line;
                const uint32_t rgb = edge ? style.outlineColor : style.color;
                auto *p = &im.pixels[(size_t(y) * im.width + x) * 4];
                p[0] = uint8_t(rgb >> 16), p[1] = uint8_t(rgb >> 8), p[2] = uint8_t(rgb);
                p[3] = uint8_t(std::lround(std::clamp(edge ? style.outlineOpacity : style.opacity, 0.0, 1.0) * 255));
            }
    }
    const int dx = (im.width - 256) / 2, dy = (im.height - 24) / 2;
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 256; ++x)
            Over(&im.pixels[(size_t(y + dy) * im.width + x + dx) * 4], &pill[(size_t(y) * 256 + x) * 4]);
    return im;
}
}  // namespace barhit
