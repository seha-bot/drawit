#include "game.hpp"

#include <cstdint>
#include <set>
#include <stdexcept>

namespace game {

void SandGrid::update_sand() noexcept {
    for (uint32_t y = height() - 1; y != static_cast<uint32_t>(-1); y--) {
        for (uint32_t x = 0; x < width(); x++) {
            auto& cell = at(x, y);
            if (cell.state != GrainState::sand) {
                continue;
            }

            utils::CellNeighbours others(*this, x, y);
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
            } else if (others.left != nullptr &&
                       others.left->state == GrainState::empty &&
                       others.bottomLeft->state == GrainState::empty) {
                *others.bottomLeft = cell;
                cell.state = GrainState::empty;
            } else if (others.right != nullptr &&
                       others.right->state == GrainState::empty &&
                       others.bottomRight->state == GrainState::empty) {
                *others.bottomRight = cell;
                cell.state = GrainState::empty;
                x++;
            }
        }
    }
}

void SandGrid::place_solid(const Solid& solid) {
    currentSolid.emplace(solid);

    for (uint32_t y = 0; y < solid.texture.height(); y++) {
        for (uint32_t x = 0; x < solid.texture.width(); x++) {
            const uint8_t pixel = solid.texture.at(x, y) & 0xFF;
            if (pixel == 0) {
                continue;
            }

            auto& grain = at(solid.x + x, solid.y + y);
            if (grain.state != GrainState::empty) {
                throw game_over_error();
            }

            grain = {GrainState::solid, pixel, solid.color};
        }
    }
}

void SandGrid::remove_current_solid() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error(
            "trying to remove a solid when there are none");
    }

    for (uint32_t y = 0; y < currentSolid->texture.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->texture.width(); x++) {
            if (currentSolid->texture.at(x, y) == 0) {
                continue;
            }
            at(currentSolid->x + x, currentSolid->y + y) = Grain::empty();
        }
    }
    currentSolid.reset();
}

void SandGrid::move_current_solid(Direction direction) {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("trying to move a solid when there are none");
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
            if (currentSolid->x + currentSolid->texture.width() != width()) {
                newX++;
            }
            break;
        case Direction::up:
            if (currentSolid->y != 0) {
                newY--;
            }
            break;
        case Direction::down:
            if (currentSolid->y + currentSolid->texture.height() != height()) {
                newY++;
            }
            break;
    }

    auto solid = currentSolid.value();
    remove_current_solid();
    solid.x = newX;
    solid.y = newY;
    place_solid(solid);
}

static bool does_solid_fit(const SandGrid& grid, const Solid& solid) noexcept {
    for (uint32_t y = 0; y < solid.texture.height(); y++) {
        for (uint32_t x = 0; x < solid.texture.width(); x++) {
            if (solid.texture.at(x, y) == 0) {
                continue;
            }

            if (solid.x + x >= grid.width() || solid.y + y >= grid.height()) {
                return false;
            }

            if (grid.at(solid.x + x, solid.y + y).state == GrainState::sand) {
                return false;
            }
        }
    }

    return true;
}

void SandGrid::rotate_current_solid() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("Trying to check a solid when there are none");
    }

    auto solid = currentSolid.value();
    solid.texture = solid.texture.ror();

    if (does_solid_fit(*this, solid)) {
        remove_current_solid();
        place_solid(solid);
    }
}

bool SandGrid::does_current_solid_collide() const {
    if (!currentSolid.has_value()) {
        throw std::runtime_error("trying to check a solid when there are none");
    }

    if (currentSolid->y + currentSolid->texture.height() == height()) {
        return true;
    }

    for (uint32_t y = 0; y < currentSolid->texture.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->texture.width(); x++) {
            if (currentSolid->texture.at(x, y) == 0) {
                continue;
            }
            // const_cast is safe here because data is never modified
            const utils::CellNeighbours others(const_cast<SandGrid&>(*this),
                                               currentSolid->x + x,
                                               currentSolid->y + y);

            if ((others.left != nullptr &&
                 others.left->state == GrainState::sand) ||
                (others.right != nullptr &&
                 others.right->state == GrainState::sand) ||
                (others.top != nullptr &&
                 others.top->state == GrainState::sand) ||
                (others.bottom != nullptr &&
                 others.bottom->state == GrainState::sand)) {
                return true;
            }
        }
    }
    return false;
}

void SandGrid::convert_current_solid_to_sand() {
    if (!currentSolid.has_value()) {
        throw std::runtime_error(
            "trying to convert a solid when there are none");
    }

    for (uint32_t y = 0; y < currentSolid->texture.height(); y++) {
        for (uint32_t x = 0; x < currentSolid->texture.width(); x++) {
            if (currentSolid->texture.at(x, y) == 0) {
                continue;
            }
            at(currentSolid->x + x, currentSolid->y + y).state =
                GrainState::sand;
        }
    }
}

static bool does_area_hit_right_border(
    const SandGrid& grid, uint32_t color, uint32_t x, uint32_t y,
    std::set<std::pair<uint32_t, uint32_t>>& checked) noexcept {
    if (x >= grid.width() || y >= grid.height()) {
        return false;
    }

    const auto& cell = grid.at(x, y);
    if (checked.insert({x, y}).second == false ||
        cell.state != GrainState::sand || cell.color != color) {
        return false;
    }

    return x == grid.width() - 1 ||
           does_area_hit_right_border(grid, color, x + 1, y, checked) ||
           does_area_hit_right_border(grid, color, x, y + 1, checked) ||
           does_area_hit_right_border(grid, color, x, y - 1, checked) ||
           does_area_hit_right_border(grid, color, x - 1, y - 1, checked) ||
           does_area_hit_right_border(grid, color, x + 1, y - 1, checked) ||
           does_area_hit_right_border(grid, color, x - 1, y + 1, checked) ||
           does_area_hit_right_border(grid, color, x + 1, y + 1, checked);
}

std::optional<uint32_t> get_any_area_id(const SandGrid& grid) noexcept {
    std::optional<uint32_t> currentColor;

    for (uint32_t y = 0; y < grid.height(); y++) {
        const auto& cell = grid.at(0, y);
        if (cell.state != GrainState::sand || currentColor == cell.color) {
            continue;
        }

        std::set<std::pair<uint32_t, uint32_t>> checked;
        if (does_area_hit_right_border(grid, cell.color, 0, y, checked)) {
            return y;
        }

        currentColor.emplace(cell.color);
    }

    return std::nullopt;
}

static unsigned rm_area(SandGrid& grid, uint32_t color, uint32_t x,
                        uint32_t y) noexcept {
    if (x >= grid.width() || y >= grid.height()) {
        return 0;
    }

    auto& cell = grid.at(x, y);
    if (cell.state == GrainState::empty || cell.color != color) {
        return 0;
    }
    cell.state = GrainState::empty;

    return rm_area(grid, color, x + 1, y) + rm_area(grid, color, x, y + 1) +
           rm_area(grid, color, x, y - 1) + rm_area(grid, color, x - 1, y - 1) +
           rm_area(grid, color, x + 1, y - 1) +
           rm_area(grid, color, x - 1, y + 1) +
           rm_area(grid, color, x + 1, y + 1) + 1;
}

unsigned remove_area(SandGrid& grid, uint32_t id) {
    const auto& cell = grid.at(0, id);
    if (cell.state != GrainState::sand) {
        throw std::runtime_error("Trying to remove a non-sand area");
    }

    return rm_area(grid, cell.color, 0, id);
}

}  // namespace game
