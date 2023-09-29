#ifndef MASKHPP
#define MASKHPP

// TODO clean these includes
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
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

    void rotate() noexcept {
        // TODO unimplemented
        return;

        auto maskRot(mask);

        for (size_t y = 0; y < _height / 2; y++) {
            for (size_t x = 0; x < _width / 2; x++) {
                // mask.at(x + y * _width) = maskRot.at(y + x * _height);
                size_t id = x + y * _width;
                size_t otherX = id / _width;
                size_t otherY = id % _width;
                mask.at(id) = maskRot.at(otherX + otherY * _height);
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
