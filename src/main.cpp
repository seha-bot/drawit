#include "sand.hpp"
#include <chrono>
#include <unistd.h>

void replaceCheck(SandGrid& grid, double& solidTick, const Mask& mask) {
    if (grid.doesCurrentSolidTouchSandOrBottom()) {
        grid.convertCurrentSolidToSand();
        solidTick = -1.0;
        grid.placeSolid({ &mask, static_cast<uint32_t>(rand() & 0xFFFFFF), 0, 0 });
    }
}

int main() {
    std::srand(std::time(nullptr));
    const int gridWidth = 80;
    const int gridHeight = 180;
    const int cellSize = 3;

    drw::Window window(gridWidth * cellSize, gridHeight * cellSize);
    SandGrid grid(gridWidth, gridHeight, cellSize);

    const Mask mask("mask.pgm");
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
                    replaceCheck(grid, solidTick, mask);
                } else if (event->keycode == 114) {
                    grid.moveCurrentSolid(Direction::right);
                    replaceCheck(grid, solidTick, mask);
                } else if (event->keycode == 116) {
                    grid.moveCurrentSolid(Direction::down);
                    replaceCheck(grid, solidTick, mask);
                }
            }
        }

        sandTick += deltaTime;
        if (sandTick > 0.01) {
            sandTick -= 0.01;
            grid.updateSand();
            replaceCheck(grid, solidTick, mask);
        }

        solidTick += deltaTime;
        if (solidTick > 0.03) {
            solidTick -= 0.03;
            grid.moveCurrentSolid(Direction::down);
            replaceCheck(grid, solidTick, mask);
        }

        grid.draw(window);
        window.flush();
    }
}
