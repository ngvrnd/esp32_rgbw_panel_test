#pragma once
#include <cstdint>
struct Rgbw {
    std::uint8_t red{}, green{}, blue{}, white{};
    constexpr bool operator==(const Rgbw &) const = default;
};
