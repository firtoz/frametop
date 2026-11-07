// The drag bar's visible pill stays 256x24. Only its laser hit height grows; the
// control row moves down enough that even the enlarged target stays below the panel.
#pragma once

#include <algorithm>
#include <cmath>

namespace barhit {
constexpr double kDefaultScale = 1, kMaxScale = 5;
constexpr double kAspect = 24.0 / 256;

inline double Scale(double scale) {
    return std::isfinite(scale) ? std::clamp(scale, kDefaultScale, kMaxScale) : kDefaultScale;
}
inline double Height(double width, double scale) { return width * kAspect * Scale(scale); }
inline double Below(double width, double button, double scale) {
    return width * 0.06 + std::max(Height(width, scale) / 2, button / 2);
}
}  // namespace barhit
