#include "digital_rain.hpp"

#include <algorithm>

DigitalRain::DigitalRain(DigitalRainParameters parameters) : parameters_(parameters)
{
    const DigitalRainParameters defaults{};
    if (!parameters_.step_us) parameters_.step_us = defaults.step_us;
    if (!parameters_.trail_length) parameters_.trail_length = defaults.trail_length;
    if (!parameters_.head_green) parameters_.head_green = defaults.head_green;
}

bool DigitalRain::start(PanelIndex panel)
{
    return panel < max_panels;
}

bool DigitalRain::render(PanelIndex panel, const EffectContext &context,
                         FrameBuffer &frame)
{
    if (panel >= max_panels) return false;
    frame.clear();
    const std::uint64_t base_tick = context.elapsed_us / parameters_.step_us;
    const std::uint32_t travel = FrameBuffer::height + parameters_.trail_length
                               + parameters_.gap_rows;

    for (std::size_t x = 0; x < FrameBuffer::width; ++x) {
        const auto column_seed = hash(static_cast<std::uint32_t>(panel) * 8U
                                      + static_cast<std::uint32_t>(x));
        const std::uint8_t speed_divisor = 1U + static_cast<std::uint8_t>((column_seed >> 8U) % 3U);
        const std::uint32_t phase = column_seed % travel;
        const std::uint32_t head = static_cast<std::uint32_t>((base_tick / speed_divisor + phase) % travel);

        for (std::size_t y = 0; y < FrameBuffer::height; ++y) {
            if (head < y) continue;
            const auto distance = head - static_cast<std::uint32_t>(y);
            if (distance > parameters_.trail_length) continue;
            const auto green = static_cast<std::uint8_t>(std::max<unsigned>(
                1U, parameters_.head_green / (distance + 1U)));
            frame.at(x, y) = {0, green, 0, 0};
        }
    }
    return true;
}

std::uint32_t DigitalRain::hash(std::uint32_t value)
{
    value += 0x9e3779b9U;
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    return value ^ (value >> 16U);
}
