#include "expanding_rings.hpp"
#include <algorithm>
namespace {
constexpr std::uint8_t active_white=16,retained_white=1;
std::uint8_t progress8(std::uint64_t elapsed,std::uint64_t duration){
    if(duration==0||elapsed>=duration)return 255;
    return static_cast<std::uint8_t>((elapsed*255U+duration/2U)/duration);
}
Rgbw scaled(Rgbw c,std::uint8_t mix){const auto s=[mix](std::uint8_t v){return static_cast<std::uint8_t>((static_cast<unsigned>(v)*mix+127U)/255U);};return{s(c.red),s(c.green),s(c.blue),s(c.white)};}
}
ExpandingRings::ExpandingRings(ExpandingRingsParameters p,RingColorFunction colors):parameters_(p),colors_(colors?colors:default_color){
    const ExpandingRingsParameters defaults{};
    if(!parameters_.fade_us)parameters_.fade_us=defaults.fade_us;
    if(!parameters_.endpoint_hold_us)parameters_.endpoint_hold_us=defaults.endpoint_hold_us;
    if(!parameters_.transition_us)parameters_.transition_us=defaults.transition_us;
}
bool ExpandingRings::start(PanelIndex panel){return panel<max_panels;}
bool ExpandingRings::render(PanelIndex panel,const EffectContext &context,FrameBuffer &frame){
    if(panel>=max_panels)return false;
    const auto phase=locate(context.elapsed_us);const auto mix=progress8(phase.elapsed_us,phase.duration_us);
    switch(phase.spec.kind){
    case PhaseKind::ramp_up:draw_ring(1,1,mix,frame);break;
    case PhaseKind::hold:draw_ring(phase.spec.to,phase.spec.to,255,frame);break;
    case PhaseKind::transition:draw_transition(phase.spec.from,phase.spec.to,mix,frame);break;
    case PhaseKind::ramp_down:draw_ring(1,1,static_cast<std::uint8_t>(255U-mix),frame);break;}
    return true;
}
std::uint64_t ExpandingRings::cycle_duration_us()const{std::uint64_t total{};for(const auto&p:phases_)total+=duration(p);return total;}
std::uint32_t ExpandingRings::duration(const PhaseSpec&p)const{switch(p.kind){case PhaseKind::ramp_up:case PhaseKind::ramp_down:return parameters_.fade_us;case PhaseKind::hold:return parameters_.endpoint_hold_us;case PhaseKind::transition:return parameters_.transition_us;}return 1;}
ExpandingRings::LocatedPhase ExpandingRings::locate(std::uint64_t elapsed)const{auto within=elapsed%cycle_duration_us();for(const auto&p:phases_){const auto d=duration(p);if(within<d)return{p,within,d};within-=d;}return{phases_.back(),0,duration(phases_.back())};}
std::uint8_t ExpandingRings::ring_index(std::size_t x,std::size_t y){const auto dx=x<3?3-x:(x>4?x-4:0);const auto dy=y<3?3-y:(y>4?y-4:0);return static_cast<std::uint8_t>(std::max(dx,dy)+1);}
Rgbw ExpandingRings::default_color(std::uint8_t index,bool active){return index==0||index>4?Rgbw{}:Rgbw{0,0,0,active?active_white:retained_white};}
void ExpandingRings::draw_transition(std::uint8_t from,std::uint8_t to,std::uint8_t mix,FrameBuffer&frame)const{const auto extent=std::max(from,to);for(std::size_t y=0;y<frame.height;++y)for(std::size_t x=0;x<frame.width;++x){const auto i=ring_index(x,y);if(i==from)frame.at(x,y)=scaled(colors_(i,true),255U-mix);else if(i==to)frame.at(x,y)=scaled(colors_(i,true),mix);else if(i<extent)frame.at(x,y)=colors_(i,false);else frame.at(x,y)=colors_(0,false);}}
void ExpandingRings::draw_ring(std::uint8_t extent,std::uint8_t active,std::uint8_t mix,FrameBuffer&frame)const{for(std::size_t y=0;y<frame.height;++y)for(std::size_t x=0;x<frame.width;++x){const auto i=ring_index(x,y);if(i==active)frame.at(x,y)=scaled(colors_(i,true),mix);else if(i<extent)frame.at(x,y)=colors_(i,false);else frame.at(x,y)=colors_(0,false);}}
