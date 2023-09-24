#include "sand.hpp"

void SandGrid::draw(drw::Window& window) const noexcept {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const auto& cell = at(x, y);
            const auto col = cell.state == GrainState::empty ? 0 : cell.color;
            window.drawRect(x * cellSize, y * cellSize, cellSize, cellSize, col);
        }
    }
}

#ifdef SANDSIMALT
void SandGrid::updateSand() noexcept {
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
#else
void SandGrid::updateSand() noexcept {
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

            if (others.bottom->state == GrainState::empty) {
                *others.bottom = cell;
                cell.state = GrainState::empty;
            } else if (others.left != nullptr && others.left->state == GrainState::empty && others.bottomLeft->state == GrainState::empty) {
                if (goDown) {
                    *others.bottomLeft = cell;
                } else {
                    *others.left = cell;
                }
                cell.state = GrainState::empty;
            } else if (others.right != nullptr && others.right->state == GrainState::empty && others.bottomRight->state == GrainState::empty) {
                if (goDown) {
                    *others.bottomRight = cell;
                } else {
                    *others.right = cell;
                }
                cell.state = GrainState::empty;
                x++;
            }
        }
    }
}
#endif /* SANDSIMALT */

void SandGrid::placeSolid(const Solid& solid) {
    currentSolid = solid;
    for (uint32_t y = 0; y < solid.mask->height(); y++) {
        for (uint32_t x = 0; x < solid.mask->width(); x++) {
            const uint32_t pixel = solid.mask->at(x, y);
            if (pixel == 0) {
                continue;
            }

            auto& grain = at(solid.x + x, solid.y + y);
            if (grain.state != GrainState::empty) {
                throw std::runtime_error("Trying to draw over a non-empty grain");
            }

            const auto r = (solid.color >> 16 & 0xFF) * pixel / 255;
            const auto g = (solid.color >> 8 & 0xFF) * pixel / 255;
            const auto b = (solid.color & 0xFF) * pixel / 255;

            grain = Grain(GrainState::solid, r << 16 | g << 8 | b);
        }
    }
}

void SandGrid::moveCurrentSolid(Direction direction) {
    auto newX = currentSolid.x;
    auto newY = currentSolid.y;

    switch (direction) {
        case Direction::left:
            if (currentSolid.x != 0) {
                newX--;
            }
        break;
        case Direction::right:
            if (currentSolid.x + currentSolid.mask->width() != width) {
                newX++;
            }
        break;
        case Direction::up:
            if (currentSolid.y != 0) {
                newY--;
            }
        break;
        case Direction::down:
            if (currentSolid.y + currentSolid.mask->height() != height) {
                newY++;
            }
        break;
    }

    for (uint32_t y = 0; y < currentSolid.mask->height(); y++) {
        for (uint32_t x = 0; x < currentSolid.mask->width(); x++) {
            if (currentSolid.mask->at(x, y) == 0) {
                continue;
            }

            auto& grain = at(currentSolid.x + x, currentSolid.y + y);
            if (grain.state == GrainState::sand) {
                throw std::runtime_error("Trying to draw over a non-empty grain");
            }

            grain = Grain();
        }
    }

    currentSolid.x = newX;
    currentSolid.y = newY;
    placeSolid(currentSolid);
}

bool SandGrid::doesCurrentSolidTouchSandOrBottom() const noexcept {
    if (currentSolid.y + currentSolid.mask->height() == height) {
        return true;
    }

    for (uint32_t y = 0; y < currentSolid.mask->height(); y++) {
        for (uint32_t x = 0; x < currentSolid.mask->width(); x++) {
            if (currentSolid.mask->at(x, y) == 0) {
                continue;
            }
            // const_cast is safe here because data is never modified
            const CellNeighbours others(const_cast<SandGrid&>(*this), currentSolid.x + x, currentSolid.y + y);

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

void SandGrid::convertCurrentSolidToSand() noexcept {
    for (uint32_t y = 0; y < currentSolid.mask->height(); y++) {
        for (uint32_t x = 0; x < currentSolid.mask->width(); x++) {
            if (currentSolid.mask->at(x, y) == 0) {
                continue;
            }
            at(currentSolid.x + x, currentSolid.y + y).state = GrainState::sand;
        }
    }
}
