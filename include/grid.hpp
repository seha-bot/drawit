#ifndef GRIDHPP
#define GRIDHPP

#include <cstdint>
#include <stdexcept>
#include <vector>

template<typename T>
class Grid {
    std::vector<T> cells;
public:
    uint32_t width;
    uint32_t height;

    const T& at(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) {
            throw std::runtime_error("Cell position out of bounds");
        }
        return cells[x + y * width];
    }

    T& at(uint32_t x, uint32_t y) {
        if (x >= width || y >= height) {
            throw std::runtime_error("Cell position out of bounds");
        }
        return cells[x + y * width];
    }

    Grid(uint32_t width, uint32_t height) : cells(width * height), width(width), height(height) {}
};

template<typename T>
class CellNeighbours {
public:
    T * const top;
    T * const bottom;
    T * const left;
    T * const right;
    T * const topLeft;
    T * const topRight;
    T * const bottomLeft;
    T * const bottomRight;

    CellNeighbours(Grid<T>& grid, uint32_t x, uint32_t y)
        : top(y > 0 ? &grid.at(x, y - 1) : nullptr),
        bottom(y < grid.height - 1 ? &grid.at(x, y + 1) : nullptr),
        left(x > 0 ? &grid.at(x - 1, y) : nullptr),
        right(x < grid.width - 1 ? &grid.at(x + 1, y) : nullptr),
        topLeft(top != nullptr && left != nullptr ? &grid.at(x - 1, y - 1) : nullptr),
        topRight(top != nullptr && right != nullptr ? &grid.at(x + 1, y - 1) : nullptr),
        bottomLeft(bottom != nullptr && left != nullptr ? &grid.at(x - 1, y + 1) : nullptr),
        bottomRight(bottom != nullptr && right != nullptr ? &grid.at(x + 1, y + 1) : nullptr)
    {}
};

#endif /* GRIDHPP */
