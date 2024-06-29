#ifndef SIMULATIONHPP
#define SIMULATIONHPP

#include <cstdint>
#include <optional>

#include "grid.hpp"
#include "texture.hpp"

namespace game {

enum class GrainState { empty, solid, sand };
enum class Direction { left, right, up, down };

struct Grain {
    GrainState state;
    uint8_t mask;
    uint32_t color;

    // TODO remove this
    static Grain empty() { return {GrainState::empty, 0, 0}; }
};

struct Solid {
    utils::PostProcessedTexture texture;
    uint32_t color;
    uint32_t x;
    uint32_t y;
};

// I am sorry for using exceptions for control flow :((
struct game_over_error {};

class SandGrid : public utils::Grid<Grain> {
    std::optional<Solid> currentSolid;

public:
    void update_sand() noexcept;

    void place_solid(const Solid& solid);
    void remove_current_solid();
    void move_current_solid(Direction direction);
    void rotate_current_solid();

    bool does_current_solid_collide() const;
    void convert_current_solid_to_sand();

    SandGrid(uint32_t width, uint32_t height)
        : Grid(width, height, Grain::empty()) {}
};

std::optional<uint32_t> get_any_area_id(const SandGrid& grid) noexcept;
unsigned remove_area(SandGrid& grid, uint32_t id);

}  // namespace game

#endif
