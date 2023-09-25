#include "config.hpp"
#include "sand.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>

static Solid generateRandomSolid(uint32_t x, uint32_t y) {
    return { cfg::masks[rand() % cfg::masks.size()], cfg::maskColors[rand() % cfg::maskColors.size()], x, y };
}

static void replaceCheck(SandGrid& grid, double& solidTick) {
    if (grid.doesCurrentSolidTouchSandOrBottom()) {
        grid.convertCurrentSolidToSand();
        solidTick = -1.0;
        grid.placeSolid(generateRandomSolid(0, 0));
    }
}

int main() {
    std::srand(std::time(nullptr));

    drw::Window window(cfg::gridWidth * cfg::cellSize, cfg::gridHeight * cfg::cellSize);
    SandGrid grid(cfg::gridWidth, cfg::gridHeight, cfg::cellSize);

    grid.placeSolid(generateRandomSolid(0, 0));

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
                    replaceCheck(grid, solidTick);
                } else if (event->keycode == 114) {
                    grid.moveCurrentSolid(Direction::right);
                    replaceCheck(grid, solidTick);
                } else if (event->keycode == 116) {
                    grid.moveCurrentSolid(Direction::down);
                    replaceCheck(grid, solidTick);
                } else if (event->keycode == 65) {
                    solidTick = sandTick = -10000;
                }
            }
        }

        sandTick += deltaTime;
        if (sandTick > 0.01) {
            sandTick -= 0.01;
            grid.updateSand();
            replaceCheck(grid, solidTick);

            const auto y = grid.getAnyAreaHeight();
            // std::cout << y.has_value() << std::endl;
            if (y.has_value()) {
                grid.removeAreaAtHeight(y.value());
            }
        }

        solidTick += deltaTime;
        if (solidTick > 0.03) {
            solidTick -= 0.03;
            grid.moveCurrentSolid(Direction::down);
            replaceCheck(grid, solidTick);
        }

        grid.draw(window);
        window.flush();
    }
}
