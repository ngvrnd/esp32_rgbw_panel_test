#pragma once
#include <cstddef>
#include <cstdint>
struct PanelMapping{std::uint8_t quarter_turns{};bool mirror_x{},mirror_y{},serpentine{};};
constexpr std::size_t map_pixel(std::size_t x,std::size_t y,PanelMapping mapping){mapping.quarter_turns%=4;for(std::uint8_t turn=0;turn<mapping.quarter_turns;++turn){const auto old_x=x;x=7-y;y=old_x;}if(mapping.mirror_x)x=7-x;if(mapping.mirror_y)y=7-y;if(mapping.serpentine&&(y&1U))x=7-x;return y*8+x;}
