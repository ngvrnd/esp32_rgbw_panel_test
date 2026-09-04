#pragma once
#include <array>
#include <cstdint>
#include "effect.hpp"
using RingColorFunction = Rgbw (*)(std::uint8_t index, bool is_ring_step);
struct ExpandingRingsParameters {
    std::uint32_t fade_us = 56'667;
    std::uint32_t endpoint_hold_us = 640'000;
    std::uint32_t transition_us = 430'000;
};
class ExpandingRings final : public Effect {
public:
    static constexpr std::size_t max_panels = 6;
    explicit ExpandingRings(ExpandingRingsParameters parameters = {}, RingColorFunction colors = default_color);
    const char *name() const override { return "expanding-rings"; }
    bool start(PanelIndex panel) override;
    bool render(PanelIndex panel, const EffectContext &context, FrameBuffer &frame) override;
    std::uint64_t cycle_duration_us() const;
    static std::uint8_t ring_index(std::size_t x, std::size_t y);
    static Rgbw default_color(std::uint8_t index, bool is_ring_step);
private:
    enum class PhaseKind : std::uint8_t { ramp_up, hold, transition, ramp_down };
    struct PhaseSpec { PhaseKind kind; std::uint8_t from, to; };
    struct LocatedPhase { PhaseSpec spec; std::uint64_t elapsed_us, duration_us; };
    static constexpr std::array<PhaseSpec, 11> phases_{{
        {PhaseKind::ramp_up,0,1},{PhaseKind::hold,1,1},{PhaseKind::transition,1,2},
        {PhaseKind::transition,2,3},{PhaseKind::transition,3,4},{PhaseKind::hold,4,4},
        {PhaseKind::transition,4,3},{PhaseKind::transition,3,2},{PhaseKind::transition,2,1},
        {PhaseKind::hold,1,1},{PhaseKind::ramp_down,1,0},
    }};
    std::uint32_t duration(const PhaseSpec &phase) const;
    LocatedPhase locate(std::uint64_t elapsed_us) const;
    void draw_ring(std::uint8_t extent,std::uint8_t active,std::uint8_t mix,FrameBuffer &frame) const;
    void draw_transition(std::uint8_t from,std::uint8_t to,std::uint8_t mix,FrameBuffer &frame) const;
    ExpandingRingsParameters parameters_;
    RingColorFunction colors_;
};
