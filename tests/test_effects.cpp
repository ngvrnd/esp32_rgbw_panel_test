#include <cassert>
#include <cstddef>
#include <iostream>
#include "effect_manager.hpp"
#include "digital_rain.hpp"
#include "debounced_button.hpp"
#include "expanding_rings.hpp"
#include "panel_mapping.hpp"

namespace {
std::size_t count(const FrameBuffer &frame, Rgbw color){std::size_t result{};for(const auto&p:frame.pixels())if(p==color)++result;return result;}
class CapturingSink final:public FrameSink{public:bool show(PanelIndex panel,const FrameBuffer&frame)override{last_panel=panel;last_frame=frame;++calls;return succeeds;}PanelIndex last_panel{};FrameBuffer last_frame{};unsigned calls{};bool succeeds{true};};
class ContextProbe final:public Effect{public:const char*name()const override{return"probe";}bool start(PanelIndex panel)override{started_panel=panel;return panel<6;}bool render(PanelIndex panel,const EffectContext&value,FrameBuffer&frame)override{rendered_panel=panel;context=value;frame.clear({1,2,3,4});return true;}PanelIndex started_panel{},rendered_panel{};EffectContext context{};};
}

int main(){
    FrameBuffer frame;frame.clear({1,2,3,4});assert(count(frame,{1,2,3,4})==64);
    const std::size_t expected[]={0,4,12,20,28};
    for(std::uint8_t ring=1;ring<=4;++ring){std::size_t n{};for(std::size_t y=0;y<8;++y){for(std::size_t x=0;x<8;++x){if(ExpandingRings::ring_index(x,y)==ring)++n;}}assert(n==expected[ring]);}

    ExpandingRings effect({100,200,300});assert(effect.cycle_duration_us()==2'600);assert(effect.start(0));assert(!effect.start(6));
    assert(effect.render(0,{0,0},frame));assert(count(frame,{})==64);
    effect.render(0,{50,50},frame);assert(count(frame,{0,0,0,8})==4);
    effect.render(0,{100,50},frame);assert(count(frame,{0,0,0,16})==4);
    effect.render(0,{300,200},frame);assert(count(frame,{0,0,0,16})==4);
    effect.render(0,{450,150},frame);assert(count(frame,{0,0,0,8})==16);
    effect.render(0,{600,150},frame);assert(count(frame,{0,0,0,1})==4);assert(count(frame,{0,0,0,16})==12);

    FrameBuffer direct,after_extra_calls;effect.render(0,{1'237,1'237},direct);
    effect.render(0,{10,10},after_extra_calls);effect.render(0,{700,690},after_extra_calls);effect.render(0,{1'237,537},after_extra_calls);
    assert(direct.pixels()==after_extra_calls.pixels());effect.render(0,{0,0},direct);
    effect.render(0,{effect.cycle_duration_us(),999},after_extra_calls);assert(direct.pixels()==after_extra_calls.pixels());
    frame.clear({9,9,9,9});assert(!effect.render(6,{0,0},frame));assert(count(frame,{9,9,9,9})==64);

    DigitalRain rain({70'000,4,3,12});
    FrameBuffer rain_direct,rain_repeated,rain_next,rain_other_panel;
    assert(rain.start(0));assert(!rain.start(6));
    assert(rain.render(0,{350'000,350'000},rain_direct));
    rain.render(0,{0,0},rain_repeated);rain.render(0,{140'000,140'000},rain_repeated);
    rain.render(0,{350'000,210'000},rain_repeated);
    assert(rain_direct.pixels()==rain_repeated.pixels());
    rain.render(0,{420'000,70'000},rain_next);
    assert(rain_direct.pixels()!=rain_next.pixels());
    rain.render(1,{350'000,350'000},rain_other_panel);
    assert(rain_direct.pixels()!=rain_other_panel.pixels());
    std::size_t lit{};
    for(const auto &pixel:rain_direct.pixels()){
        assert(pixel.red==0&&pixel.blue==0&&pixel.white==0);
        assert(pixel.green<=12);
        if(pixel.green)++lit;
    }
    assert(lit>0&&lit<FrameBuffer::pixel_count);
    rain_direct.clear({9,9,9,9});assert(!rain.render(6,{0,0},rain_direct));
    assert(count(rain_direct,{9,9,9,9})==64);

    CapturingSink sink;ContextProbe probe;EffectManager manager(sink,1);
    assert(manager.assign(0,&probe,1'000));assert(!manager.assign(1,&probe,1'000));assert(manager.render(1'250));
    assert(probe.context.elapsed_us==250&&probe.context.delta_us==0);assert(manager.render(1'900));
    assert(probe.context.elapsed_us==900&&probe.context.delta_us==650);assert(sink.calls==2&&sink.last_panel==0);
    assert(count(sink.last_frame,{1,2,3,4})==64);assert(manager.clear(0));assert(manager.render(2'000));assert(count(sink.last_frame,{})==64);

    assert(map_pixel(0,0,{})==0);assert(map_pixel(7,0,{})==7);assert(map_pixel(0,1,{0,false,false,true})==15);
    assert(map_pixel(0,0,{1,false,false,false})==7);assert(map_pixel(0,0,{0,true,true,false})==63);

    DebouncedButton button(30);
    assert(!button.update(false,0));
    assert(!button.update(true,10));
    assert(!button.update(false,20));
    assert(!button.update(true,30));
    assert(!button.update(true,59));
    assert(button.update(true,60));
    assert(!button.update(true,100));
    assert(!button.update(false,110));
    assert(!button.update(false,140));
    assert(!button.update(true,150));
    assert(button.update(true,180));

    CapturingSink switch_sink;EffectManager switch_manager(switch_sink,1);
    assert(switch_manager.assign(0,&rain,10'000));
    assert(switch_manager.render(80'000));
    const auto progressed=rain_direct=switch_manager.frame(0);
    assert(switch_manager.assign(0,&effect,80'000));
    assert(switch_manager.render(80'000));
    assert(count(switch_manager.frame(0),{})==64);
    assert(progressed.pixels()!=switch_manager.frame(0).pixels());
    std::cout<<"All timestamp-driven C++ effect tests passed\n";
}
