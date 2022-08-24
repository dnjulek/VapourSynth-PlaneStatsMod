#include "shared.h"

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi) {
	vspapi->configPlugin("com.julek.psm", "psm", "PlaneStats with threshold", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
	vspapi->registerFunction("PlaneMinMax", "clip:vnode;minthr:float:opt;maxthr:float:opt;plane:int:opt;", "clip:vnode;", planeMinMaxCreate, nullptr, plugin);
	vspapi->registerFunction("PlaneAverage", "clip:vnode;value_exclude:int[];plane:int:opt;prop:data:opt;", "clip:vnode;", planeAverageCreate, nullptr, plugin);
}