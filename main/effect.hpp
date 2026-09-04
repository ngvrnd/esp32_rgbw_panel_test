#pragma once
#include <cstdint>
#include "frame_buffer.hpp"
using PanelIndex = std::uint8_t;
struct EffectContext { std::uint32_t elapsed_ms{}, delta_ms{}; };
class Effect {
public:
    virtual ~Effect() = default;
    virtual const char *name() const = 0;
    virtual void start(PanelIndex panel, std::uint32_t now_ms) = 0;
    virtual void render(PanelIndex panel, const EffectContext &context, FrameBuffer &frame) = 0;
};
