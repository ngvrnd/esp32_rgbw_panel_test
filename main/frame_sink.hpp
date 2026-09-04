#pragma once
#include "effect.hpp"
class FrameSink { public: virtual ~FrameSink()=default; virtual bool show(PanelIndex panel,const FrameBuffer&frame)=0; };
