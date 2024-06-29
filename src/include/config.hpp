#ifndef CONFIGHPP
#define CONFIGHPP

#include <cstdint>
#include <tuple>
#include <vector>

#include "texture.hpp"

namespace cfg {

static const std::vector<uint32_t> maskColors({0x89FC00, 0xF5B700, 0xDC0073,
                                               0x008BF8});

static utils::Color shader(int32_t x, int32_t y,
                           const utils::PostProcessedTexture& texture) {
    utils::Color c(texture.utils::Texture::at(x, y));
    const auto [r, g, b] = c.asDouble();
    static const double brightnessDark = 0.6;

    try {
        if (texture.utils::Texture::at(x + 1, y) == 0 ||
            texture.utils::Texture::at(x, y + 1) == 0 ||
            texture.utils::Texture::at(x + 1, y + 1) == 0) {
            c = std::make_tuple(r * brightnessDark, g * brightnessDark,
                                b * brightnessDark);
        }
    } catch (...) {
        c = std::make_tuple(r * brightnessDark, g * brightnessDark,
                            b * brightnessDark);
    }

    return c;
}

static const std::vector<utils::PostProcessedTexture> masks({
    {utils::P3Parser::parse("assets/mask1.ppm"), shader},
    {utils::P3Parser::parse("assets/mask2.ppm"), shader},
    {utils::P3Parser::parse("assets/mask3.ppm"), shader},
    {utils::P3Parser::parse("assets/mask4.ppm"), shader},
    {utils::P3Parser::parse("assets/mask5.ppm"), shader},
});

}  // namespace cfg

#endif
