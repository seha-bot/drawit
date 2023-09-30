#include "config.hpp"
#include "drawit.hpp"
#include "sand.hpp"
#include <chrono>
#include <unistd.h>

static Solid generateRandomSolid(uint32_t x, uint32_t y) {
    return { cfg::masks[rand() % cfg::masks.size()], cfg::maskColors[rand() % cfg::maskColors.size()], x, y };
}

// TODO this name is unclear
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

    while (!window.isClosed() && !window.isKeyDown(drw::Key::escape)) {
        const auto now = std::chrono::system_clock::now();
        const auto subDeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() * 0.001;
        if (subDeltaTime < targetFPS) {
            usleep((targetFPS - subDeltaTime) * 1000000.0);
        }
        lastTime = std::chrono::system_clock::now();
        const auto deltaTime = std::max(subDeltaTime, targetFPS);

        window.pollEvents();
        if (window.isKeyDown(drw::Key::left)) {
            grid.moveCurrentSolid(Direction::left);
            replaceCheck(grid, solidTick);
        }
        if (window.isKeyDown(drw::Key::right)) {
            grid.moveCurrentSolid(Direction::right);
            replaceCheck(grid, solidTick);
        }
        if (window.isKeyDown(drw::Key::down)) {
            grid.moveCurrentSolid(Direction::down);
            replaceCheck(grid, solidTick);
        }
        if (window.isKeyDown(drw::Key::space)) {
            solidTick = sandTick = -10000;
        }
        if (window.isKeyDownOnce(drw::Key::up)) {
            grid.rotateCurrentSolid();
        }

        sandTick += deltaTime;
        if (sandTick > 0.01) {
            sandTick -= 0.01;
            grid.updateSand();
            replaceCheck(grid, solidTick);

            const auto y = grid.getAnyAreaHeight();
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
