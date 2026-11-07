// The invisible laser target grows, not the visible pill; it never covers content.
#include "../barhit.h"

#include <cstdio>
#include <limits>

int main() {
    int failed = 0;
    auto check = [&](const char *what, bool ok) {
        std::printf("%s %s\n", ok ? "ok" : "FAIL", what);
        failed += !ok;
    };
    auto near = [](double a, double b) { return std::fabs(a - b) < 1e-10; };
    for (double width : {0.03, 0.3, 1.5}) {
        for (double button : {width * 0.13, width * 0.8}) {
            check("default keeps the original control row", near(barhit::Below(width, button, 1),
                  width * 0.06 + std::max(width * 12 / 256, button / 2)));
            for (double scale : {1.0, 1.25, 3.0, 5.0}) {
                const double h = barhit::Height(width, scale), below = barhit::Below(width, button, scale);
                check("target scales with the pill", near(h, width * 24 / 256 * scale));
                check("target stays below the panel", below - h / 2 >= width * 0.06 - 1e-10);
                check("buttons stay below the panel", below - button / 2 >= width * 0.06 - 1e-10);
                check("pill stays inside the hit target", h >= width * 24 / 256);
            }
        }
    }
    check("scale is bounded", barhit::Scale(-1) == 1 && barhit::Scale(100) == 5);
    check("NaN uses the default", barhit::Scale(std::numeric_limits<double>::quiet_NaN()) == 1);
    check("infinity uses the default", barhit::Scale(std::numeric_limits<double>::infinity()) == 1);
    return failed ? 1 : 0;
}
