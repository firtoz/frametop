#include "../resizecorner.h"
#include <cassert>
#include <iostream>

int main() {
    constexpr int n = 256;
    resizecorner::Style style;
    for (double length : {1.0, 4.0, 10.0})
        for (double width : {0.5, 2.0, 6.0}) {
            style.lengthCm = length, style.widthMm = width, style.color = 0x12abef;
            const double size = resizecorner::Canvas(style);
            const auto base = resizecorner::Texture(n, size, 1, -1, style);
            for (int sx : {-1, 1}) for (int sy : {-1, 1}) {
                const auto px = resizecorner::Texture(n, size, sx, sy, style);
                int solid = 0, clear = 0, aa = 0;
                for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
                    const auto i = (size_t(y) * n + x) * 4;
                    const int a = px[i + 3];
                    solid += a == 255, clear += a == 0, aa += a > 0 && a < 255;
                    assert(px[i] == 0x12 && px[i + 1] == 0xab && px[i + 2] == 0xef);
                    if (sx * (x + 0.5 - n / 2) < 0 && sy * (n / 2 - y - 0.5) < 0) assert(a == 0);
                    const int bx = sx == 1 ? x : n - 1 - x, by = sy == -1 ? y : n - 1 - y;
                    assert(a == base[(size_t(by) * n + bx) * 4 + 3]);
                    if (x == 0 || y == 0 || x == n - 1 || y == n - 1) assert(a == 0);
                }
                if (width / 1000 >= 2 * size / n) assert(solid > 0);
                assert(aa > 0 && clear > n * n * 0.8);
            }
        }
    for (double gap : {0.0, 2.0, 10.0}) {
        style.gapCm = gap;
        const double size = resizecorner::Canvas(style);
        const auto idle = resizecorner::Texture(n, size, 1, -1, style);
        const auto ready = resizecorner::Texture(n, size, 1, -1, style, true);
        int idleArea = 0, readyArea = 0;
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
            const auto i = (size_t(y) * n + x) * 4;
            idleArea += idle[i + 3] > 0, readyArea += ready[i + 3] > 0;
            if (ready[i + 3]) {
                assert(ready[i] == uint8_t(style.hoverColor >> 16));
                if (x == 0 || y == 0 || x == n - 1 || y == n - 1) assert(false);
                const double X = (x + 0.5 - n / 2.0) * size / n;
                const double Y = (y + 0.5 - n / 2.0) * size / n;
                const double distance = std::hypot(std::max(X, 0.0), std::max(Y, 0.0));
                assert(distance >= gap / 100 - size / n);
            }
        }
        assert(readyArea > idleArea);
    }
    std::cout << "ok crisp corners: orientations, gap clearance, bounded canvas, AA, distinct thicker hover state\n";
}
