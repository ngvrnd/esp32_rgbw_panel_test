#pragma once
#include <cstdint>
#include "frame_buffer.hpp"
using PanelIndex = std::uint8_t;
struct EffectContext { std::uint64_t elapsed_us{}, delta_us{}; };
class Effect {
public:
    virtual ~Effect() = default;
    virtual const char *name() const = 0;
    virtual bool start(PanelIndex panel) = 0;
    virtual bool render(PanelIndex panel, const EffectContext &context, FrameBuffer &frame) = 0;
};
