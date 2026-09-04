#pragma once
#include <array>
#include <cstddef>
#include "color.hpp"
class FrameBuffer {
public:
    static constexpr std::size_t width = 8, height = 8, pixel_count = width * height;
    void clear(Rgbw color = {}) { pixels_.fill(color); }
    Rgbw &at(std::size_t x, std::size_t y) { return pixels_.at(y * width + x); }
    const Rgbw &at(std::size_t x, std::size_t y) const { return pixels_.at(y * width + x); }
    const std::array<Rgbw, pixel_count> &pixels() const { return pixels_; }
private:
    std::array<Rgbw, pixel_count> pixels_{};
};
