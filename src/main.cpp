#include "drawit.hpp"
#include <algorithm>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

class Drawable {
public:
    uint32_t width;
    uint32_t height;

    virtual void draw(drw::Window& window, uint32_t x, uint32_t y) const noexcept = 0;

    Drawable(uint32_t width, uint32_t height) : width(width), height(height) {}

    Drawable(const Drawable&) = default;
    Drawable& operator=(const Drawable&) = default;
    virtual ~Drawable() {}
};

class Grain {
public:
    enum State { empty, solid, sand };

    State state;
    uint32_t color;
    Grain(State state, uint32_t color) : state(state), color(color) {}
    Grain() : state(State::empty), color(0) {}
};

template<typename T>
class GridNeighbours {
public:
    T *top;
    T *bottom;
    T *left;
    T *right;
    T *topLeft;
    T *topRight;
    T *bottomLeft;
    T *bottomRight;

    GridNeighbours(T *cells, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
        : top(y > 0 ? cells + x + width * y - width : nullptr),
        bottom(y < height - 1 ? cells + x + width * y + width : nullptr),
        left(x > 0 ? cells + x - 1 + y * width : nullptr),
        right(x < width - 1 ? cells + x + 1 + y * width : nullptr),
        topLeft(top != nullptr && left != nullptr ? cells + x - 1 + width * y - width : nullptr),
        topRight(top != nullptr && right != nullptr ? cells + x + 1 + width * y - width : nullptr),
        bottomLeft(bottom != nullptr && left != nullptr ? cells + x - 1 + width * y + width : nullptr),
        bottomRight(bottom != nullptr && right != nullptr ? cells + x + 1 + width * y + width : nullptr)
    {}
};

class Mask {
    const std::vector<uint8_t> mask;
public:
    const uint32_t width;
    const uint32_t height;

    // uint32_t colorAt(uint32_t x, uint32_t y) const noexcept {
    //     return colors[(x % width) + (y % height) * width];
    // }

    uint8_t operator[](size_t i) const noexcept {
        return mask.at(i);
    }

    Mask(const std::vector<uint8_t>& mask, uint32_t width, uint32_t height) : mask(mask), width(width), height(height) {}
};

struct Solid {
    const Mask* mask;
    uint32_t color;
    uint32_t x;
    uint32_t y;
};

enum Direction { left, right, up, down };

class SandGrid : public Drawable {
    const uint16_t cellSize;
    std::vector<Grain> cells;

    Solid currentSolid;
public:
    void draw(drw::Window& window, uint32_t x, uint32_t y) const noexcept override {
        for (uint32_t yi = 0; yi < height; yi++) {
            for (uint32_t xi = 0; xi < width; xi++) {
                const auto cell = cells[xi + yi * width];
                const auto col = cell.state == Grain::State::empty ? 0 : cell.color;
                window.drawRect(x + xi * cellSize, y + yi * cellSize, cellSize, cellSize, col);
            }
        }
    }

    void updateSand() noexcept {
        for (uint32_t y = height - 1; y != static_cast<uint32_t>(-1); y--) {
            for (uint32_t x = 0; x < width; x++) {
                auto& cell = cells[x + y * width];
                if (cell.state != Grain::State::sand) {
                    continue;
                }

                GridNeighbours others(cells.data(), width, height, x, y);
                if (others.bottom == nullptr) {
                    continue;
                }

                const bool goDown = std::rand() & 1;

                if (others.bottom->state == Grain::State::empty) {
                    *others.bottom = cell;
                    cell.state = Grain::State::empty;
                } else if (others.left != nullptr && others.left->state == Grain::State::empty && others.bottomLeft->state == Grain::State::empty) {
                    if (goDown) {
                        *others.bottomLeft = cell;
                    } else {
                        *others.left = cell;
                    }
                    cell.state = Grain::State::empty;
                } else if (others.right != nullptr && others.right->state == Grain::State::empty && others.bottomRight->state == Grain::State::empty) {
                    if (goDown) {
                        *others.bottomRight = cell;
                    } else {
                        *others.right = cell;
                    }
                    cell.state = Grain::State::empty;
                    x++;
                }
            }
        }
    }

    Grain& at(uint32_t x, uint32_t y) {
        if (x >= width || y >= height) {
            throw std::runtime_error("Grain position out of bounds");
        }
        return cells[x + y * width];
    }

    void placeSolid(const Solid& solid) {
        currentSolid = solid;
        for (uint32_t yi = 0; yi < solid.mask->height; yi++) {
            for (uint32_t xi = 0; xi < solid.mask->width; xi++) {
                const uint32_t pixel = solid.mask->operator[](xi + yi * solid.mask->width);
                if (pixel == 0) {
                    continue;
                }

                auto& grain = at(solid.x + xi, solid.y + yi);
                if (grain.state != Grain::State::empty) {
                    throw std::runtime_error("Trying to draw over a non-empty grain");
                }

                const auto r = (solid.color >> 16 & 0xFF) * pixel / 255;
                const auto g = (solid.color >> 8 & 0xFF) * pixel / 255;
                const auto b = (solid.color & 0xFF) * pixel / 255;

                grain = Grain(Grain::State::solid, r << 16 | g << 8 | b);
            }
        }
    }

    // TODO add direction enum and do bounds check inside.
    void moveCurrentSolid(Direction direction) {
        auto newX = currentSolid.x;
        auto newY = currentSolid.y;

        switch (direction) {
            case Direction::left:
                if (currentSolid.x != 0) {
                    newX--;
                }
            break;
            case Direction::right:
                if (currentSolid.x + currentSolid.mask->width != width) {
                    newX++;
                }
            break;
            case Direction::up:
                if (currentSolid.y != 0) {
                    newY--;
                }
            break;
            case Direction::down:
                if (currentSolid.y + currentSolid.mask->height != height) {
                    newY++;
                }
            break;
        }

        for (uint32_t yi = 0; yi < currentSolid.mask->height; yi++) {
            for (uint32_t xi = 0; xi < currentSolid.mask->width; xi++) {
                if (currentSolid.mask->operator[](xi + yi * currentSolid.mask->width) == 0) {
                    continue;
                }

                auto& grain = at(currentSolid.x + xi, currentSolid.y + yi);
                if (grain.state == Grain::State::sand) {
                    throw std::runtime_error("Trying to draw over a non-empty grain");
                }

                grain = Grain();
            }
        }

        currentSolid.x = newX;
        currentSolid.y = newY;
        placeSolid(currentSolid);
    }

    bool doesCurrentSolidTouchSandOrBottom() const noexcept {
        if (currentSolid.y + currentSolid.mask->height == height) {
            return true;
        }

        for (uint32_t yi = 0; yi < currentSolid.mask->height; yi++) {
            for (uint32_t xi = 0; xi < currentSolid.mask->width; xi++) {
                if (currentSolid.mask->operator[](xi + yi * currentSolid.mask->width) == 0) {
                    continue;
                }
                const GridNeighbours others(cells.data(), width, height, currentSolid.x + xi, currentSolid.y + yi);

                if ((others.left != nullptr && others.left->state == Grain::State::sand)
                    || (others.right != nullptr && others.right->state == Grain::State::sand)
                    || (others.top != nullptr && others.top->state == Grain::State::sand)
                    || (others.bottom != nullptr && others.bottom->state == Grain::State::sand)) {
                    return true;
                }
            }
        }
        return false;
    }

    void convertCurrentSolidToSand() noexcept {
        for (uint32_t yi = 0; yi < currentSolid.mask->height; yi++) {
            for (uint32_t xi = 0; xi < currentSolid.mask->width; xi++) {
                if (currentSolid.mask->operator[](xi + yi * currentSolid.mask->width) == 0) {
                    continue;
                }
                at(currentSolid.x + xi, currentSolid.y + yi).state = Grain::State::sand;
            }
        }
    }

    SandGrid(uint32_t width, uint32_t height, uint16_t cellSize) : Drawable(width, height), cellSize(cellSize), cells(width * height) {}
};

int main() {
    // std::srand(std::time(nullptr));
    const int gridWidth = 80;
    const int gridHeight = 180;
    const int cellSize = 3;

    drw::Window window(gridWidth * cellSize, gridHeight * cellSize);
    SandGrid grid(gridWidth, gridHeight, cellSize);

    int w = 20, h = 20;
    std::vector<uint8_t> maskData(w*h);
    for (size_t i = 0; i < maskData.size(); i++) { maskData[i] = ((i & 1) + 1) * 120; }
    const Mask mask(maskData, w, h);
    grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });

    const auto targetFPS = 1.0 / 60.0;
    auto lastTime = std::chrono::system_clock::now();

    double sandTick = 0.0;
    double solidTick = 0.0;

    while (true) {
        const auto now = std::chrono::system_clock::now();
        const auto subDeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() * 0.001;
        if (subDeltaTime < targetFPS) {
            usleep((targetFPS - subDeltaTime) * 1000000.0);
        }
        lastTime = std::chrono::system_clock::now();
        const auto deltaTime = std::max(subDeltaTime, targetFPS);

        std::optional<drw::Event> event;
        while ((event = window.nextEvent())) {
            // TODO make proper key press events
            if (event->type == drw::EventType::windowUnmapped) {
                return 0;
            } else if (event->type == drw::EventType::keyPress) {
                if (event->keycode == 66 || event->keycode == 24) {
                    return 0;
                } else if (event->keycode == 113) {
                    grid.moveCurrentSolid(Direction::left);
                    if (grid.doesCurrentSolidTouchSandOrBottom()) {
                        grid.convertCurrentSolidToSand();
                        solidTick = -1.0;
                        grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
                    }
                } else if (event->keycode == 114) {
                    grid.moveCurrentSolid(Direction::right);
                    if (grid.doesCurrentSolidTouchSandOrBottom()) {
                        grid.convertCurrentSolidToSand();
                        solidTick = -1.0;
                        grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
                    }
                } else if (event->keycode == 116) {
                    // TODO handle speeding better
                    grid.moveCurrentSolid(Direction::down);
                    if (grid.doesCurrentSolidTouchSandOrBottom()) {
                        grid.convertCurrentSolidToSand();
                        solidTick = -1.0;
                        grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
                    }
                }
            }
        }

        sandTick += deltaTime;
        if (sandTick > 0.01) {
            sandTick -= 0.01;
            grid.updateSand();
            if (grid.doesCurrentSolidTouchSandOrBottom()) {
                grid.convertCurrentSolidToSand();
                solidTick = -1.0;
                grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
            }
        }

        solidTick += deltaTime;
        if (solidTick > 0.03) {
            solidTick -= 0.03;
            grid.moveCurrentSolid(Direction::down);
            // TODO extract all of these in a checker function
            if (grid.doesCurrentSolidTouchSandOrBottom()) {
                grid.convertCurrentSolidToSand();
                solidTick = -1.0;
                grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
            }
        }

        grid.draw(window, 0, 0);
        window.flush();
    }
}
