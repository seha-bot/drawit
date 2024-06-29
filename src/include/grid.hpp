#ifndef GRIDHPP
#define GRIDHPP

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace utils {

template <typename T>
class Grid {
    std::vector<T> m_cells;
    uint32_t m_width;
    uint32_t m_height;

public:
    Grid(uint32_t width, uint32_t height, const T& allocator = T()) noexcept
        : m_cells(width * height, allocator),
          m_width(width),
          m_height(height) {}

    Grid(const std::vector<T>& data, uint32_t width, uint32_t height) noexcept
        : m_cells(data), m_width(width), m_height(height) {
        assert(data.size() == width * height);
    }

    uint32_t width() const noexcept { return m_width; }
    uint32_t height() const noexcept { return m_height; }

    const T& at(uint32_t x, uint32_t y) const {
        if (x >= m_width || y >= m_height) {
            throw std::runtime_error("Cell position out of bounds");
        }
        return m_cells[x + y * m_width];
    }

    T& at(uint32_t x, uint32_t y) {
        return const_cast<T&>(static_cast<const Grid<T> *>(this)->at(x, y));
    }
};

// TODO remove this as it sucks
template <typename T>
struct CellNeighbours {
    T *const top;
    T *const bottom;
    T *const left;
    T *const right;
    T *const topLeft;
    T *const topRight;
    T *const bottomLeft;
    T *const bottomRight;

    CellNeighbours(Grid<T>& grid, uint32_t x, uint32_t y)
        : top(y > 0 ? &grid.at(x, y - 1) : nullptr),
          bottom(y < grid.height() - 1 ? &grid.at(x, y + 1) : nullptr),
          left(x > 0 ? &grid.at(x - 1, y) : nullptr),
          right(x < grid.width() - 1 ? &grid.at(x + 1, y) : nullptr),
          topLeft(top != nullptr && left != nullptr ? &grid.at(x - 1, y - 1)
                                                    : nullptr),
          topRight(top != nullptr && right != nullptr ? &grid.at(x + 1, y - 1)
                                                      : nullptr),
          bottomLeft(bottom != nullptr && left != nullptr
                         ? &grid.at(x - 1, y + 1)
                         : nullptr),
          bottomRight(bottom != nullptr && right != nullptr
                          ? &grid.at(x + 1, y + 1)
                          : nullptr) {}
};

}  // namespace utils

#endif
