#include <assert.h>
#include <stdio.h>
#include "ring_effect.h"
static ring_color_t palette(uint8_t i,bool active)
{
    if(!i)return(ring_color_t){0,0,0,0};
    return(ring_color_t){i,active?10:1,0,active?20:2};
}
int main(void)
{
    assert(RING_EFFECT_DEFAULT_INNER_DWELL==192);
    assert(RING_EFFECT_DEFAULT_STEP_DWELL==128);
    int count[5]={0};for(int y=0;y<8;y++)for(int x=0;x<8;x++)count[ring_effect_index_for_xy(x,y)]++;
    assert(count[0]==0&&count[1]==4&&count[2]==12&&count[3]==20&&count[4]==28);
    assert(ring_effect_index_for_xy(3,3)==1&&ring_effect_index_for_xy(0,0)==4);
    ring_color_t c=ring_effect_default_color(2,true);assert(c.white==16&&!c.red&&!c.green&&!c.blue);
    assert(ring_effect_default_color(2,false).white==1);
    assert(ring_effect_default_color(0,false).white==0&&ring_effect_default_color(5,true).white==0);
    c=ring_effect_color_for_pixel(3,3,3,2,255,3,0,palette);assert(c.red==1&&c.green==1&&c.white==2);
    c=ring_effect_color_for_pixel(2,2,2,2,128,3,127,palette);assert(c.red==1&&c.green==5&&c.white==10);
    c=ring_effect_color_for_pixel(0,0,3,2,255,3,0,palette);assert(c.red==0&&c.white==0);
    puts("ring model tests passed");return 0;
}
