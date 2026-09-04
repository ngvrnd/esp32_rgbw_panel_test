#pragma once
#include <array>
#include <cstdint>
#include "effect.hpp"
using RingColorFunction = Rgbw (*)(std::uint8_t index, bool is_ring_step);
class ExpandingRings final : public Effect {
public:
    static constexpr std::uint16_t default_inner_dwell = 192;
    static constexpr std::uint16_t default_step_dwell = 128;
    static constexpr std::size_t max_panels = 6;
    explicit ExpandingRings(std::uint16_t inner_dwell = default_inner_dwell,
        std::uint16_t step_dwell = default_step_dwell,
        RingColorFunction colors = default_color);
    const char *name() const override { return "expanding-rings"; }
    void start(PanelIndex panel, std::uint32_t now_ms) override;
    void render(PanelIndex panel, const EffectContext &, FrameBuffer &frame) override;
    static std::uint8_t ring_index(std::size_t x, std::size_t y);
    static Rgbw default_color(std::uint8_t index, bool is_ring_step);
private:
    enum class Stage : std::uint8_t { ramp_up, hold_center_out, out_1_2, out_2_3,
        out_3_4, hold_outer, in_4_3, in_3_2, in_2_1, hold_center_in, ramp_down };
    struct State { Stage stage{Stage::ramp_up}; std::uint16_t frame{}; std::uint32_t started_ms{}; };
    std::uint16_t stage_length(Stage stage) const;
    void advance(State &state) const;
    void draw(const State &state, FrameBuffer &frame) const;
    void draw_transition(std::uint8_t from, std::uint8_t to, std::uint8_t mix, FrameBuffer &frame) const;
    void draw_ring(std::uint8_t extent, std::uint8_t active, std::uint8_t mix, FrameBuffer &frame) const;
    std::uint16_t inner_dwell_, step_dwell_;
    RingColorFunction colors_;
    std::array<State, max_panels> states_{};
};
