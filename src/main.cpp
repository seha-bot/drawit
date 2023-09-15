#include "drawit.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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
    uint8_t id;
    Grain(State state, uint32_t color, uint8_t id) : state(state), color(color), id(id) {}
    Grain() : state(State::empty), color(0), id(0) {}
};

class GrainNeighbours {
public:
    Grain *bottom;
    Grain *left;
    Grain *right;
    Grain *bottomLeft;
    Grain *bottomRight;

    GrainNeighbours(const Drawable& drawable, Grain *grains, uint32_t x, uint32_t y) : left(nullptr), right(nullptr) {
        const auto bottomColumn = y * drawable.width + drawable.width;
        bottom = y < drawable.height - 1 ? grains + x + bottomColumn : nullptr;
        if (x > 0) {
            left = grains + x - 1 + y * drawable.width;
            bottomLeft = bottom == nullptr ? nullptr : grains + x - 1 + bottomColumn;
        }
        if (x < drawable.width - 1) {
            right = grains + x + 1 + y * drawable.width;
            bottomRight = bottom == nullptr ? nullptr : grains + x + 1 + bottomColumn;
        }
    }
};

class SandGrid : public Drawable {
    const uint16_t cellSize;
    Grain *cells;
public:
    uint32_t solidX;
    uint32_t solidY;

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
                auto cell = cells + x + y * width;
                if (cell->state != Grain::State::sand) {
                    continue;
                }

                GrainNeighbours others(*this, cells, x, y);
                if (others.bottom == nullptr) {
                    continue;
                }

                const bool goDown = std::rand() & 1;

                if (others.bottom->state == Grain::State::empty) {
                    *others.bottom = *cell;
                    cell->state = Grain::State::empty;
                } else if (others.left != nullptr && others.left->state == Grain::State::empty && others.bottomLeft->state == Grain::State::empty) {
                    if (goDown) {
                        *others.bottomLeft = *cell;
                    } else {
                        *others.left = *cell;
                    }
                    cell->state = Grain::State::empty;
                } else if (others.right != nullptr && others.right->state == Grain::State::empty && others.bottomRight->state == Grain::State::empty) {
                    if (goDown) {
                        *others.bottomRight = *cell;
                    } else {
                        *others.right = *cell;
                    }
                    cell->state = Grain::State::empty;
                    x++;
                }
            }
        }
    }

    Grain& grainAt(uint32_t x, uint32_t y) {
        if (x >= width || y >= height) {
            throw std::runtime_error("Grain position out of bounds");
        }
        return cells[x + y * width];
    }

    SandGrid(uint32_t width, uint32_t height, uint16_t cellSize) : Drawable(width, height), cellSize(cellSize) {
        cells = new Grain[width * height];
        temp();
    }

    void temp() {
        for (uint32_t yi = 0; yi < height; yi++) {
            for (uint32_t xi = 0; xi < width; xi++) {
                if (yi > 10 && yi < 30 && xi > 30 && xi < 50) {
                    grainAt(xi, yi) = { Grain::State::sand, std::rand()&0xFFFFFFU, 0 };
                }
            }
        }
    }

    SandGrid(const SandGrid&) = delete;
    SandGrid& operator=(const SandGrid&) = delete;
    ~SandGrid() {
        delete[] cells;
    }
};

class Texture {
    uint32_t *colors;
public:
    const uint32_t width;
    const uint32_t height;

    uint32_t colorAt(uint32_t x, uint32_t y) const noexcept {
        return colors[(x % width) + (y % height) * width];
    }

    Texture(uint32_t *colors, uint32_t width, uint32_t height) : colors(colors), width(width), height(height) {}
};

class Shape {
    uint8_t *mask;
public:
    const uint32_t width;
    const uint32_t height;

    // TODO maybe instead of this enum just throw
    enum ApplyStatus { overdraw, ok };
    ApplyStatus applyTo(SandGrid& grid, uint32_t x, uint32_t y, const Texture& texture, uint8_t id) const noexcept {
        for (uint32_t yi = 0; yi < height; yi++) {
            for (uint32_t xi = 0; xi < width; xi++) {
                if (mask[xi + yi * width] == 0) {
                    continue;
                }

                auto& grain = grid.grainAt(x + xi, y + yi);
                if (grain.state != Grain::State::empty) {
                    return ApplyStatus::overdraw;
                }
                grain = Grain(Grain::State::solid, texture.colorAt(xi, yi), id);
            }
        }
        return ApplyStatus::ok;
    }

    void removeFrom(SandGrid& grid, uint32_t x, uint32_t y) const noexcept {
        for (uint32_t yi = 0; yi < height; yi++) {
            for (uint32_t xi = 0; xi < width; xi++) {
                if (mask[xi + yi * width] == 0) {
                    continue;
                }
                grid.grainAt(x + xi, y + yi) = Grain();
            }
        }
    }

    Shape(uint8_t *mask, uint32_t width, uint32_t height) : mask(mask), width(width), height(height) {}
};

int main() {
    std::srand(std::time(nullptr));
    const int gridWidth = 80;
    const int gridHeight = 180;
    const int cellSize = 3;

    drw::Window window(gridWidth * cellSize, gridHeight * cellSize);
    SandGrid grid(gridWidth, gridHeight, cellSize);

    auto mask = new uint8_t[20 * 20];
    for (int i = 0; i < 20*20; i++) {mask[i] = 1;}
    Shape box(mask, 20, 20);

    auto colors = new uint32_t[20*20];
    for (int i = 0; i < 20*20; i++) {
        colors[i] = i % 2 * 255 << 8 | (i+1) % 2 * 255;
    }
    Texture tex(colors, 20, 20);

    uint32_t boxX = 0, boxY = 0;

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
            if (event->type == drw::EventType::windowUnmapped) {
                return 0;
            } else if (event->type == drw::EventType::keyPress) {
                std::cout << event->keycode << std::endl;
                if (event->keycode == 66 || event->keycode == 24) {
                    return 0;
                } else if (event->keycode == 38) {
                    box.applyTo(grid, 0, 0, tex, 0);
                } else if (event->keycode == 65) {
                    grid.updateSand();
                } else if (event->keycode == 113) {
                    if (boxX != 0) {
                        box.removeFrom(grid, boxX, boxY);
                        box.applyTo(grid, --boxX, boxY, tex, 0);
                    }
                } else if (event->keycode == 114) {
                    if (boxX + box.width != grid.width - 1) {
                        box.removeFrom(grid, boxX, boxY);
                        box.applyTo(grid, ++boxX, boxY, tex, 0);
                    }
                }
            }
        }

        sandTick += deltaTime;
        if (sandTick > 0.01) {
            sandTick -= 0.01;
            grid.updateSand();
        }

        solidTick += deltaTime;
        if (solidTick > 0.05) {
            solidTick -= 0.05;
            box.removeFrom(grid, boxX, boxY);
            box.applyTo(grid, boxX, ++boxY, tex, 0);
        }

        grid.draw(window, 0, 0);
        window.flush();
    }
}
