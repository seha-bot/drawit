#include <SDL_events.h>
#include <SDL_scancode.h>
#include <SDL_timer.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "config.hpp"
#include "game.hpp"
#include "kiss.hpp"
#include "kiss_sdl.h"
#include "texture.hpp"

// TODO remove
double second = 0.0;

static game::Solid gen_random_solid(uint32_t x) {
    return {cfg::masks[rand() % cfg::masks.size()],
            cfg::maskColors[rand() % cfg::maskColors.size()], x, 0};
}

struct GameState {
    double sand_tick = 0.0;
    double solid_tick = 0.0;
    uint32_t start_time = SDL_GetTicks();
    double score = 0.0;
    game::Solid next_solid;
    bool game_over = false;

    GameState(game::Solid&& initial_next_solid)
        : next_solid(initial_next_solid) {}
};

static void collision_resolution(game::SandGrid& grid, GameState& state) {
    if (grid.does_current_solid_collide()) {
        grid.convert_current_solid_to_sand();
        grid.place_solid(state.next_solid);
        state.next_solid = gen_random_solid(grid.width() / 3);
    }
}

static auto game_render(game::SandGrid& grid) {
    return [&grid](kiss::Canvas& canvas) {
        for (uint32_t y = 0; y < grid.height(); y++) {
            for (uint32_t x = 0; x < grid.width(); x++) {
                const auto& cell = grid.at(x, y);

                const auto col =
                    cell.state == game::GrainState::empty ? 0 : cell.color;
                const auto [r, g, b] = utils::Color(col).asDouble();

                canvas.set_pixel(x, y, r * cell.mask, g * cell.mask,
                                 b * cell.mask);
            }
        }
    };
}

static void game_logic(kiss::Window& w, kiss::KeyboardListener& k,
                       game::SandGrid& grid, GameState& state) {
    const auto now = SDL_GetTicks();
    const auto dt = (now - state.start_time) / 1000.0;
    state.start_time = now;
    second += dt;

    if (k.is_key_down(SDL_SCANCODE_LEFT)) {
        grid.move_current_solid(game::Direction::left);
        collision_resolution(grid, state);
        w.force_redraw();
    }
    if (k.is_key_down(SDL_SCANCODE_RIGHT)) {
        grid.move_current_solid(game::Direction::right);
        collision_resolution(grid, state);
        w.force_redraw();
    }
    if (k.is_key_down(SDL_SCANCODE_DOWN)) {
        grid.move_current_solid(game::Direction::down);
        collision_resolution(grid, state);
        state.score += dt * 5.0;
        w.force_redraw();
    }
    if (k.is_key_down_once(SDL_SCANCODE_UP)) {
        grid.rotate_current_solid();
        collision_resolution(grid, state);
        w.force_redraw();
    }

    state.sand_tick += dt;
    if (state.sand_tick > 0.02) {
        state.sand_tick -= 0.02;
        grid.update_sand();
        collision_resolution(grid, state);

        const auto id = game::get_any_area_id(grid);
        if (id.has_value()) {
            state.score += game::remove_area(grid, id.value()) / 4;
        }

        w.force_redraw();
    }

    state.solid_tick += dt;
    if (state.solid_tick > 0.01) {
        state.solid_tick -= 0.01;
        grid.move_current_solid(game::Direction::down);
        collision_resolution(grid, state);
        w.force_redraw();
    }
}

// w.register_component(std::make_unique<kiss::Button>(
//     "Click me", 50, 90, [&c] { c.set_visibility(true); }));

int main() {
    using std::make_unique;

    kiss::Window w("Tetrisand", 535, 710);
    auto& k = w.register_component(make_unique<kiss::KeyboardListener>());

    // TITLE
    w.register_component(make_unique<kiss::Label>(16, kiss_textfont.lineheight))
        .update_text("Tetrisand");

    // CANVAS
    game::SandGrid grid(80, 160);
    grid.place_solid(gen_random_solid(grid.width() / 3));
    GameState state(gen_random_solid(grid.width() / 3));

    w.register_component(make_unique<kiss::Canvas>(
        16, kiss_textfont.lineheight * 3, grid.width(), grid.height(),
        grid.width() * 4, grid.height() * 4, game_render(grid)));

    const int canvas_end_x = 16 + grid.width() * 4;

    // INFO WINDOW
    auto& info = w.register_component(make_unique<kiss::Container>(
        canvas_end_x + 16, kiss_textfont.lineheight * 3, grid.width() * 2,
        16 + kiss_textfont.lineheight * 2 + 64));
    info.set_visibility(true);

    const int info_end_y = info.y + info.h;

    // INFO WINDOW -> SCORE
    auto& score = info.register_component(
        make_unique<kiss::Label>(info.x + 8, info.y + 8));

    // INFO WINDOW -> NEXT SHAPE
    info
        .register_component(make_unique<kiss::Label>(
            info.x + 8, info.y + 8 + kiss_textfont.lineheight * 2))
        .update_text("Next:");

    w.register_component(make_unique<kiss::Canvas>(
        info.x + 8 + kiss_textfont.advance * 6,
        info.y + 8 + kiss_textfont.lineheight * 2, 24, 32, 48, 64,
        [&state](auto& canvas) {
            canvas.fill(0xFFFFFF);
            auto& tex = state.next_solid.texture;
            const auto [rc, gc, bc] =
                utils::Color(state.next_solid.color).asDouble();
            for (uint32_t y = 0; y < tex.height(); ++y) {
                for (uint32_t x = 0; x < tex.width(); ++x) {
                    if (tex.at(x, y) == 0) {
                        canvas.set_pixel(x, y, 255, 255, 255);
                        continue;
                    }
                    const auto [r, g, b] =
                        utils::Color(tex.at(x, y)).asDouble();
                    canvas.set_pixel(x, y, r * rc * 255, g * gc * 255,
                                     b * bc * 255);
                }
            }
        }));

    // CONTROLS
    w
        .register_component(make_unique<kiss::Label>(
            canvas_end_x + 16, info_end_y + kiss_textfont.lineheight))
        .update_text(
            "Controls:\n\n"
            "LEFT  -> move left\n"
            "RIGHT -> move right\n"
            "UP    -> rotate\n"
            "DOWN  -> speedup\n");

    // GAME OVER WINDOW
    auto& game_over =
        w.register_component(make_unique<kiss::Container>(133, 236, 267, 177));

    auto& game_over_label =
        game_over.register_component(make_unique<kiss::Label>(
            game_over.x + 80, game_over.y + kiss_textfont.lineheight));

    game_over.register_component(make_unique<kiss::Button>(
        "Restart", game_over.x + 100, game_over.y + 120, [&] {
            grid = game::SandGrid(grid.width(), grid.height());
            grid.place_solid(gen_random_solid(grid.width() / 3));
            state = GameState(gen_random_solid(grid.width() / 3));
            game_over.set_visibility(false);
        }));

    SDL_Event e;
    int fps = 0;
    while (w.is_open()) {
        SDL_Delay(10);

        if (second >= 1.0) {
            std::cout << fps << std::endl;
            fps = -1;
            second -= 1.0;
        }
        ++fps;

        while (SDL_PollEvent(&e)) {
            w.process_event(e);
        }

        if (!state.game_over) {
            // Again, I am sorry for using exceptions for
            // control flow :(
            try {
                game_logic(w, k, grid, state);
            } catch (const game::game_over_error&) {
                state.game_over = true;
                game_over.set_visibility(true);
                w.force_redraw();
            }
            score.update_text("Score: " +
                              std::to_string(static_cast<int>(state.score)));
            game_over_label.update_text(
                "Game over...\n\nScore: " +
                std::to_string(static_cast<int>(state.score)));
        }

        if (!w.is_ready()) {
            continue;
        }

        w.draw();
        w.flush();
    }
}
