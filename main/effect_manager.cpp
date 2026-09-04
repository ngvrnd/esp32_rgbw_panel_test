#include "effect_manager.hpp"
#include <algorithm>
EffectManager::EffectManager(FrameSink&sink,std::size_t count):sink_(sink),panel_count_(std::min(count,max_panels)){}
bool EffectManager::assign(PanelIndex panel,Effect*effect,std::uint64_t now){if(panel>=panel_count_||!effect||!effect->start(panel))return false;assignments_[panel]={effect,now,now,false};return true;}
bool EffectManager::clear(PanelIndex panel){if(panel>=panel_count_)return false;assignments_[panel]={};frames_[panel].clear();return true;}
bool EffectManager::render(std::uint64_t now){bool success=true;for(std::size_t i=0;i<panel_count_;++i){auto&a=assignments_[i];auto&out=frames_[i];if(a.effect){const auto elapsed=now>=a.assigned_us?now-a.assigned_us:0;const auto delta=a.rendered&&now>=a.last_rendered_us?now-a.last_rendered_us:0;success=a.effect->render(static_cast<PanelIndex>(i),{elapsed,delta},out)&&success;a.last_rendered_us=now;a.rendered=true;}else out.clear();success=sink_.show(static_cast<PanelIndex>(i),out)&&success;}return success;}
