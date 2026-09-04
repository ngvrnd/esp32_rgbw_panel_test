#include "expanding_rings.hpp"
#include <algorithm>
namespace {
constexpr std::uint8_t active_white = 16, retained_white = 1;
Rgbw scaled(Rgbw c, std::uint8_t mix) {
    const auto scale = [mix](std::uint8_t v) { return static_cast<std::uint8_t>((static_cast<unsigned>(v) * mix + 127U) / 255U); };
    return {scale(c.red), scale(c.green), scale(c.blue), scale(c.white)};
}
}
ExpandingRings::ExpandingRings(std::uint16_t inner, std::uint16_t step, RingColorFunction colors)
    : inner_dwell_(inner ? inner : default_inner_dwell), step_dwell_(step ? step : default_step_dwell),
      colors_(colors ? colors : default_color) {}
void ExpandingRings::start(PanelIndex panel, std::uint32_t now) {
    if (panel < states_.size()) states_[panel] = {Stage::ramp_up, 0, now};
}
void ExpandingRings::render(PanelIndex panel, const EffectContext &, FrameBuffer &frame) {
    if (panel >= states_.size()) { frame.clear(); return; }
    State &state = states_[panel]; draw(state, frame); advance(state);
}
std::uint8_t ExpandingRings::ring_index(std::size_t x, std::size_t y) {
    const auto dx = x < 3 ? 3 - x : (x > 4 ? x - 4 : 0);
    const auto dy = y < 3 ? 3 - y : (y > 4 ? y - 4 : 0);
    return static_cast<std::uint8_t>(std::max(dx, dy) + 1);
}
Rgbw ExpandingRings::default_color(std::uint8_t index, bool active) {
    return index == 0 || index > 4 ? Rgbw{} : Rgbw{0, 0, 0, active ? active_white : retained_white};
}
std::uint16_t ExpandingRings::stage_length(Stage stage) const {
    switch (stage) {
    case Stage::ramp_up: case Stage::ramp_down: return active_white + 1;
    case Stage::hold_center_out: case Stage::hold_outer: case Stage::hold_center_in: return inner_dwell_;
    default: return step_dwell_ + 1;
    }
}
void ExpandingRings::advance(State &state) const {
    if (++state.frame < stage_length(state.stage)) return;
    state.frame = 0;
    state.stage = state.stage == Stage::ramp_down ? Stage::ramp_up
        : static_cast<Stage>(static_cast<std::uint8_t>(state.stage) + 1);
}
void ExpandingRings::draw(const State &s, FrameBuffer &frame) const {
    const auto mix = [this, &s] { return static_cast<std::uint8_t>(static_cast<std::uint32_t>(s.frame) * 255U / step_dwell_); };
    switch (s.stage) {
    case Stage::ramp_up: draw_ring(1, 1, static_cast<std::uint8_t>(s.frame * 255U / active_white), frame); break;
    case Stage::hold_center_out: case Stage::hold_center_in: draw_ring(1, 1, 255, frame); break;
    case Stage::out_1_2: draw_transition(1, 2, mix(), frame); break;
    case Stage::out_2_3: draw_transition(2, 3, mix(), frame); break;
    case Stage::out_3_4: draw_transition(3, 4, mix(), frame); break;
    case Stage::hold_outer: draw_ring(4, 4, 255, frame); break;
    case Stage::in_4_3: draw_transition(4, 3, mix(), frame); break;
    case Stage::in_3_2: draw_transition(3, 2, mix(), frame); break;
    case Stage::in_2_1: draw_transition(2, 1, mix(), frame); break;
    case Stage::ramp_down: draw_ring(1, 1, static_cast<std::uint8_t>((active_white - s.frame) * 255U / active_white), frame); break;
    }
}
void ExpandingRings::draw_transition(std::uint8_t from, std::uint8_t to, std::uint8_t mix, FrameBuffer &frame) const {
    const auto extent = std::max(from, to);
    for (std::size_t y=0; y<frame.height; ++y) for (std::size_t x=0; x<frame.width; ++x) {
        const auto i=ring_index(x,y);
        if (i==from) frame.at(x,y)=scaled(colors_(i,true),255U-mix);
        else if (i==to) frame.at(x,y)=scaled(colors_(i,true),mix);
        else if (i<extent) frame.at(x,y)=colors_(i,false);
        else frame.at(x,y)=colors_(0,false);
    }
}
void ExpandingRings::draw_ring(std::uint8_t extent, std::uint8_t active, std::uint8_t mix, FrameBuffer &frame) const {
    for (std::size_t y=0; y<frame.height; ++y) for (std::size_t x=0; x<frame.width; ++x) {
        const auto i=ring_index(x,y);
        if (i==active) frame.at(x,y)=scaled(colors_(i,true),mix);
        else if (i<extent) frame.at(x,y)=colors_(i,false);
        else frame.at(x,y)=colors_(0,false);
    }
}
