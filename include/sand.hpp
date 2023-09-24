#ifndef SANDHPP
#define SANDHPP

#include "drawit.hpp"
#include "grid.hpp"
#include "mask.hpp"

class Drawable {
public:
    virtual void draw(drw::Window& window) const noexcept = 0;

    Drawable() = default;
    Drawable(const Drawable&) = default;
    Drawable& operator=(const Drawable&) = default;
    virtual ~Drawable() {}
};

enum GrainState { empty, solid, sand };

class Grain {
public:
    GrainState state;
    uint32_t color;
    Grain(GrainState state, uint32_t color) : state(state), color(color) {}
    Grain() : state(GrainState::empty), color(0) {}
};

struct Solid {
    const Mask* mask;
    uint32_t color;
    uint32_t x;
    uint32_t y;
};

enum Direction { left, right, up, down };

class SandGrid : public Drawable, public Grid<Grain> {
    const uint16_t cellSize;
    Solid currentSolid;
public:
    void draw(drw::Window& window) const noexcept override;

    void updateSand() noexcept;

    void placeSolid(const Solid& solid);
    void moveCurrentSolid(Direction direction);

    bool doesCurrentSolidTouchSandOrBottom() const noexcept;
    void convertCurrentSolidToSand() noexcept;

    SandGrid(uint32_t width, uint32_t height, uint16_t cellSize) : Grid(width, height), cellSize(cellSize) {}
};

#endif
