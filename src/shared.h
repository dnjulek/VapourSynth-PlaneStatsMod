#pragma once

#include <algorithm>
#include <vector>
#include <memory>

#include "VapourSynth4.h"
#include "VSHelper4.h"


extern void VS_CC planeMinMaxCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi);
extern void VS_CC planeAverageCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi);