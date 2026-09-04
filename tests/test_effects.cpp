#include <cassert>
#include <iostream>
#include "expanding_rings.hpp"
namespace {
EffectContext context{};
std::size_t count(const FrameBuffer &f,Rgbw c){std::size_t n{};for(const auto&p:f.pixels())if(p==c)++n;return n;}
Rgbw diagnostic(std::uint8_t i,bool active){return{i,static_cast<std::uint8_t>(active?10:1),0,0};}
void render_n(Effect&e,FrameBuffer&f,unsigned n){while(n--)e.render(0,context,f);}
}
int main(){
    FrameBuffer f;f.clear({1,2,3,4});assert(count(f,{1,2,3,4})==64);f.at(7,7)={};assert(f.pixels().back()==Rgbw{});
    const std::size_t expected[]={0,4,12,20,28};
    for(std::uint8_t r=1;r<=4;++r){
        std::size_t n{};
        for(std::size_t y=0;y<8;++y){
            for(std::size_t x=0;x<8;++x){
                if(ExpandingRings::ring_index(x,y)==r)++n;
            }
        }
        assert(n==expected[r]);
    }
    assert(ExpandingRings::default_color(1,true)==(Rgbw{0,0,0,16}));
    assert(ExpandingRings::default_color(3,false)==(Rgbw{0,0,0,1}));
    ExpandingRings e;e.start(0,0);e.render(0,context,f);assert(count(f,{})==64);render_n(e,f,16);
    assert(count(f,{0,0,0,16})==4);e.render(0,context,f);assert(count(f,{0,0,0,16})==4);
    render_n(e,f,ExpandingRings::default_inner_dwell-1);e.render(0,context,f);
    assert(count(f,{0,0,0,16})==4&&count(f,{})==60);render_n(e,f,ExpandingRings::default_step_dwell);
    assert(count(f,{})==52&&count(f,{0,0,0,16})==12);
    e.render(0,context,f);
    assert(count(f,{0,0,0,1})==4&&count(f,{0,0,0,16})==12);
    ExpandingRings custom(1,1,diagnostic);custom.start(0,0);render_n(custom,f,19);
    assert(f.at(3,3)==(Rgbw{1,10,0,0}));custom.render(0,context,f);
    assert(f.at(3,3)==Rgbw{}&&f.at(2,2)==(Rgbw{2,10,0,0}));
    f.clear({9,9,9,9});custom.render(ExpandingRings::max_panels,context,f);assert(count(f,{})==64);
    std::cout<<"All C++ effect tests passed\n";
}
