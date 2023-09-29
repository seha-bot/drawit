#ifndef SANDHPP
#define SANDHPP

#include "drawit.hpp"
#include "grid.hpp"
#include "mask.hpp"
#include <cstdint>
#include <optional>
#include <set>

// TODO generalize this in a separate hpp/cpp pair
class Drawable {
public:
    virtual void draw(drw::Window& window) const noexcept = 0;

    Drawable() = default;
    Drawable(const Drawable&) = default;
    Drawable& operator=(const Drawable&) = default;
    virtual ~Drawable() {}
};

enum GrainState { empty, solid, sand };

struct Grain {
    GrainState state;
    uint8_t mask;
    uint32_t color;
    Grain(GrainState state, uint8_t mask, uint32_t color) : state(state), mask(mask), color(color) {}
    Grain() : state(GrainState::empty), mask(0), color(0) {}
};

struct Solid {
    Mask& mask;
    uint32_t color;
    uint32_t x;
    uint32_t y;

    Solid(Mask& mask, uint32_t color, uint32_t x, uint32_t y) : mask(mask), color(color), x(x), y(y) {}
};

enum Direction { left, right, up, down };

class SandGrid : public Drawable, public Grid<Grain> {
    const uint16_t cellSize;

    std::optional<Solid> currentSolid;

    bool doesAreaHitRightBorder(uint32_t color, uint32_t x, uint32_t y, std::set<std::pair<uint32_t, uint32_t>>& checked) const noexcept;
    void removeArea(uint32_t color, uint32_t x, uint32_t y) noexcept;
public:
    void draw(drw::Window& window) const noexcept override;

    void updateSand() noexcept;

    void placeSolid(const Solid& solid);
    void removeCurrentSolid();
    void moveCurrentSolid(Direction direction);
    void rotateCurrentSolid();

    bool doesCurrentSolidTouchSandOrBottom() const;
    void convertCurrentSolidToSand();

    std::optional<uint32_t> getAnyAreaHeight() const noexcept;
    void removeAreaAtHeight(uint32_t y);

    SandGrid(uint32_t width, uint32_t height, uint16_t cellSize) : Grid(width, height), cellSize(cellSize) {}
};

#endif
