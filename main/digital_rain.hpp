#pragma once

#include <cstdint>
#include "effect.hpp"

struct DigitalRainParameters {
    std::uint32_t step_us = 70'000;
    std::uint8_t gap_rows = 4;
    std::uint8_t trail_length = 3;
    std::uint8_t head_green = 12;
};

class DigitalRain final : public Effect {
public:
    static constexpr std::size_t max_panels = 6;

    explicit DigitalRain(DigitalRainParameters parameters = {});
    const char *name() const override { return "digital-rain"; }
    bool start(PanelIndex panel) override;
    bool render(PanelIndex panel, const EffectContext &context,
                FrameBuffer &frame) override;

private:
    static std::uint32_t hash(std::uint32_t value);
    DigitalRainParameters parameters_;
};
