#ifndef MASKHPP
#define MASKHPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// TODO do not inline!!!
class Mask {
    std::vector<uint8_t> mask;
    uint32_t _width;
    uint32_t _height;
public:
    uint32_t width() const noexcept {
        return _width;
    }

    uint32_t height() const noexcept {
        return _height;
    }

    uint8_t at(uint32_t x, uint32_t y) const {
        if (x >= _width || y >= _height) {
            throw std::runtime_error("Mask position out of bounds");
        }
        return mask[x + y * _width];
    }

    void ror() noexcept {
        const auto maskRot(mask);
        for (uint32_t y = 0; y < _height; y++) {
            for (uint32_t x = 0; x < _width; x++) {
                mask[y + (_width - x - 1) * _height] = maskRot[x + y * _width];
            }
        }
        std::swap(_width, _height);
    }

    void rol() noexcept {
        const auto maskRot(mask);
        for (uint32_t y = 0; y < _height; y++) {
            for (uint32_t x = 0; x < _width; x++) {
                mask[(_height - y - 1) + x * _height] = maskRot[x + y * _width];
            }
        }
        std::swap(_width, _height);
    }

    Mask(const std::vector<uint8_t>& mask, uint32_t width, uint32_t height) : mask(mask), _width(width), _height(height) {}

    Mask(const std::string& pgmFilePath) {
        std::ifstream file(pgmFilePath);

        std::string buf;
        file >> buf;
        if (buf != "P5") {
            throw std::runtime_error("Bad image file format");
        }

        file >> _width;
        file >> _height;
        file >> buf;

        auto begin = std::istreambuf_iterator<char>(file);
        std::advance(begin, 1);
        mask.insert(mask.begin(), begin, std::istreambuf_iterator<char>());
    }
};

#endif /* MASKHPP */
