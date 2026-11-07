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
            for (double scale : {1.0, 1.25, 3.0, 5.0, 15.0}) {
                const double h = barhit::Height(width, scale), below = barhit::Below(width, button, scale);
                check("target scales with the pill", near(h, width * 24 / 256 * scale));
                check("target stays below the panel", below - h / 2 >= width * 0.06 - 1e-10);
                check("buttons stay below the panel", below - button / 2 >= width * 0.06 - 1e-10);
                check("pill stays inside the hit target", h >= width * 24 / 256);
            }
        }
    }
    check("scale is bounded", barhit::Scale(-1) == 1 && barhit::Scale(100) == 15);
    check("NaN uses the default", barhit::Scale(std::numeric_limits<double>::quiet_NaN()) == 1);
    check("infinity uses the default", barhit::Scale(std::numeric_limits<double>::infinity()) == 1);
    check("width and height independent", near(barhit::Width(0.3, 15), 4.5) && near(barhit::Height(0.3, 2), 0.05625));

    uint32_t color = 0;
    check("hex color", barhit::Color("#Aa12F0", &color) && color == 0xaa12f0);
    for (auto bad : {"red", "#123", "#12345z", "#11223344", "#112233\n"})
        check("invalid color keeps previous value", !barhit::Color(bad, &color) && color == 0xaa12f0);

    std::vector<uint8_t> pill(256 * 24 * 4, 0);
    const int source = (12 * 256 + 128) * 4;
    pill[source] = 255, pill[source + 3] = 255;
    barhit::Style style;
    auto im = barhit::Render(3, 5, style, pill);
    auto pixel = [&](int x, int y) { return &im.pixels[(size_t(y) * im.width + x) * 4]; };
    check("padded target dimensions", im.width == 768 && im.height == 120);
    check("padding transparent without preview", pixel(0, 0)[3] == 0 && pixel(20, 20)[3] == 0);
    check("pill stays centered and unchanged", pixel(im.width / 2, im.height / 2)[0] == 255 &&
          pixel(im.width / 2, im.height / 2)[3] == 255);
    style.enabled = true;
    style.color = 0x112233, style.outlineColor = 0xabcdef;
    style.opacity = 0.2, style.outlineOpacity = 1, style.outlineWidth = 2;
    im = barhit::Render(3, 5, style, pill);
    check("thin solid outline", pixel(0, 0)[0] == 0xab && pixel(1, 20)[3] == 255);
    check("semitransparent fill", pixel(2, 20)[0] == 0x11 && pixel(2, 20)[3] == 51);
    check("pill opaque over fill", pixel(im.width / 2, im.height / 2)[0] == 255 &&
          pixel(im.width / 2, im.height / 2)[3] == 255);
    style.outlineWidth = 0;
    im = barhit::Render(3, 5, style, pill);
    check("outline can be disabled", pixel(0, 0)[0] == 0x11 && pixel(0, 0)[3] == 51);
    im = barhit::Render(15, 15, style, pill);
    check("maximum canvas is bounded", im.width == 3840 && im.height == 360 && im.pixels.size() == 3840 * 360 * 4);
    return failed ? 1 : 0;
}
