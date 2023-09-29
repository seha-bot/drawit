#ifndef CONFIGHPP
#define CONFIGHPP

#include "mask.hpp"
#include <cstdint>
#include <vector>

namespace cfg {
    static const int gridWidth = 80;
    static const int gridHeight = 180;

    static const int cellSize = 3;

    static const uint32_t white  = 0xFFFFFF;
    static const uint32_t black  = 0x000000;

    static const std::vector<uint32_t> maskColors({
        0xFF0000,
        0x00FF00,
        0x0000FF,
        0x00FFFF
    });

    static std::vector<Mask> masks({
        Mask("assets/mask1.pgm"),
        Mask("assets/mask2.pgm"),
        Mask("assets/mask3.pgm"),
        Mask("assets/mask4.pgm"),
        Mask("assets/mask5.pgm")
    });
}

#endif /* CONFIGHPP */
