#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "effect.hpp"
#include "frame_sink.hpp"
class EffectManager {
public:
    static constexpr std::size_t max_panels=6;
    EffectManager(FrameSink&sink,std::size_t panel_count);
    bool assign(PanelIndex panel,Effect*effect,std::uint64_t now_us);
    bool clear(PanelIndex panel);
    bool render(std::uint64_t now_us);
    const FrameBuffer&frame(PanelIndex panel)const{return frames_.at(panel);}
private:
    struct Assignment{Effect*effect{};std::uint64_t assigned_us{},last_rendered_us{};bool rendered{};};
    FrameSink&sink_;std::size_t panel_count_;
    std::array<Assignment,max_panels>assignments_{};std::array<FrameBuffer,max_panels>frames_{};
};
