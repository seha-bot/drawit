#include "sand.hpp"
#include <stdexcept>

void sand::SandGrid::draw(drw::Window& window) const noexcept {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const auto& cell = at(x, y);
            const auto color = cell.state == GrainState::empty ? 0 : cell.color;

            const auto r = (color >> 16 & 0xFF) * cell.mask / 255;
            const auto g = (color >> 8 & 0xFF) * cell.mask / 255;
            const auto b = (color & 0xFF) * cell.mask / 255;

            window.drawRect(x * cellSize, y * cellSize, cellSize, cellSize, r << 16 | g << 8 | b);
        }
    }
}

void sand::SandGrid::updateSand() noexcept {
    for (uint32_t y = height - 1; y != static_cast<uint32_t>(-1); y--) {
        for (uint32_t x = 0; x < width; x++) {
            auto& cell = at(x, y);
            if (cell.state != GrainState::sand) {
                continue;
            }

            CellNeighbours others(*this, x, y);
            if (others.bottom == nullptr) {
                continue;
            }

            const bool goDown = std::rand() & 1;
            if (!goDown) {
                continue;
            }

            if (others.bottom->state == GrainState::empty) {
                *others.bottom = cell;
                cell.state = GrainState::empty;
            } else if (others.left != nullptr && others.left->state == GrainState::empty && others.bottomLeft->state == GrainState::empty) {
                *others.bottomLeft = cell;
                cell.state = GrainState::empty;
            } else if (others.right != nullptr && others.right->state == GrainState::empty && others.bottomRight->state == GrainState::empty) {
                *others.bottomRight = cell;
                cell.state = GrainState::empty;
                x++;
            }
        }
    }
}

void sand::SandGrid::placeSolid(const Solid& solid) {
    currentSolid.emplace(solid);

    for (uint32_t y = 0; y < solid.mask.height(); y++) {
        for (uint32_t x = 0; x < solid.mask.width(); x++) {
            const uint8_t pixel = solid.mask.at(x, y);
            if (pixel == 0) {
                continue;
            }

            auto& grain = at(solid.x + x, solid.y + y);
            if (grain.state != GrainState::empty) {
                removeCurrentSolid();
                throw std::runtime_error("Trying to draw over a non-empty grain");
            }

            grain = Grain(GrainState::solid, pixel, solid.color);
        }
    }
}

void sand::SandGrid::removeCurrentSolid() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to remove a solid when there are none");
    }

    for (uint32_t y = 0; y < currentSolid->mask.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->mask.width(); x++) {
            if (currentSolid->mask.at(x, y) == 0) {
                continue;
            }
            at(currentSolid->x + x, currentSolid->y + y) = Grain();
        }
    }
    currentSolid.reset();
}

void sand::SandGrid::moveCurrentSolid(Direction direction) {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to move a solid when there are none");
    }

    auto newX = currentSolid->x;
    auto newY = currentSolid->y;

    switch (direction) {
        case Direction::left:
            if (currentSolid->x != 0) {
                newX--;
            }
        break;
        case Direction::right:
            if (currentSolid->x + currentSolid->mask.width() != width) {
                newX++;
            }
        break;
        case Direction::up:
            if (currentSolid->y != 0) {
                newY--;
            }
        break;
        case Direction::down:
            if (currentSolid->y + currentSolid->mask.height() != height) {
                newY++;
            }
        break;
    }

    auto solid = currentSolid.value();
    removeCurrentSolid();
    solid.x = newX;
    solid.y = newY;
    placeSolid(solid);
}

static bool doesSolidFit(const sand::SandGrid& grid, const sand::Solid& solid) noexcept {
    for (uint32_t y = 0; y < solid.mask.height(); y++) {
        for (uint32_t x = 0; x < solid.mask.width(); x++) {
            const uint8_t pixel = solid.mask.at(x, y);
            if (pixel == 0) {
                continue;
            }

            if (solid.x + x >= grid.width || solid.y + y >= grid.height) {
                return false;
            }

            if (grid.at(solid.x + x, solid.y + y).state == sand::GrainState::sand) {
                return false;
            }
        }
    }

    return true;
}

void sand::SandGrid::rotateCurrentSolid() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to check a solid when there are none");
    }

    auto solid = currentSolid.value();
    solid.mask.ror();

    if (doesSolidFit(*this, solid)) {
        removeCurrentSolid();
        placeSolid(solid);
    }
}

bool sand::SandGrid::doesCurrentSolidTouchSandOrBottom() const {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to check a solid when there are none");
    }

    if (currentSolid->y + currentSolid->mask.height() == height) {
        return true;
    }

    for (uint32_t y = 0; y < currentSolid->mask.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->mask.width(); x++) {
            if (currentSolid->mask.at(x, y) == 0) {
                continue;
            }
            // const_cast is safe here because data is never modified
            const CellNeighbours others(const_cast<SandGrid&>(*this), currentSolid->x + x, currentSolid->y + y);

            if ((others.left != nullptr && others.left->state == GrainState::sand)
                || (others.right != nullptr && others.right->state == GrainState::sand)
                || (others.top != nullptr && others.top->state == GrainState::sand)
                || (others.bottom != nullptr && others.bottom->state == GrainState::sand)) {
                return true;
            }
        }
    }
    return false;
}

void sand::SandGrid::convertCurrentSolidToSand() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to convert a solid when there are none");
    }

    for (uint32_t y = 0; y < currentSolid->mask.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->mask.width(); x++) {
            if (currentSolid->mask.at(x, y) == 0) {
                continue;
            }
            at(currentSolid->x + x, currentSolid->y + y).state = GrainState::sand;
        }
    }
}

static bool doesAreaHitRightBorder(const sand::SandGrid& grid, uint32_t color, uint32_t x, uint32_t y, std::set<std::pair<uint32_t, uint32_t>>& checked) noexcept {
    if (x >= grid.width || y >= grid.height) {
        return false;
    }

    const auto& cell = grid.at(x, y);
    if (checked.insert({x, y}).second == false || cell.state != sand::GrainState::sand || cell.color != color) {
        return false;
    }

    return x == grid.width - 1
        || doesAreaHitRightBorder(grid, color, x + 1, y, checked)
        || doesAreaHitRightBorder(grid, color, x, y + 1, checked)
        || doesAreaHitRightBorder(grid, color, x, y - 1, checked)
        || doesAreaHitRightBorder(grid, color, x - 1, y - 1, checked)
        || doesAreaHitRightBorder(grid, color, x + 1, y - 1, checked)
        || doesAreaHitRightBorder(grid, color, x - 1, y + 1, checked)
        || doesAreaHitRightBorder(grid, color, x + 1, y + 1, checked);
}

std::optional<uint32_t> sand::getAnyAreaId(const SandGrid& grid) noexcept {
    std::optional<uint32_t> currentColor;

    for (uint32_t y = 0; y < grid.height; y++) {
        const auto& cell = grid.at(0, y);
        if (cell.state != GrainState::sand || currentColor == cell.color) {
            continue;
        }

        std::set<std::pair<uint32_t, uint32_t>> checked;
        if (doesAreaHitRightBorder(grid, cell.color, 0, y, checked)) {
            return y;
        }

        currentColor.emplace(cell.color);
    }

    return std::nullopt;
}

static void removeAreaRecursive(sand::SandGrid& grid, uint32_t color, uint32_t x, uint32_t y) noexcept {
    if (x >= grid.width || y >= grid.height) {
        return;
    }

    auto& cell = grid.at(x, y);
    if (cell.state == sand::GrainState::empty || cell.color != color) {
        return;
    }
    cell.state = sand::GrainState::empty;

    removeAreaRecursive(grid, color, x + 1, y);
    removeAreaRecursive(grid, color, x, y + 1);
    removeAreaRecursive(grid, color, x, y - 1);
    removeAreaRecursive(grid, color, x - 1, y - 1);
    removeAreaRecursive(grid, color, x + 1, y - 1);
    removeAreaRecursive(grid, color, x - 1, y + 1);
    removeAreaRecursive(grid, color, x + 1, y + 1);
}

void sand::removeArea(SandGrid& grid, uint32_t id) {
    const auto& cell = grid.at(0, id);
    if (cell.state != GrainState::sand) {
        throw std::runtime_error("Trying to remove a non-sand area");
    }

    removeAreaRecursive(grid, cell.color, 0, id);
}
