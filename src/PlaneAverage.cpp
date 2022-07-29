#include "shared.h"

struct planeAverageData final {
	VSNode* node;
	const VSVideoInfo* vi;
	std::vector<int> pxlist;
	int plane;
};


static const VSFrame* VS_CC planeAverageGetFrame(int n, int activationReason, void* instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi) {
	auto d{ static_cast<planeAverageData*>(instanceData) };

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		const VSFrame* src = vsapi->getFrameFilter(n, d->node, frameCtx);
		VSFrame* dst = vsapi->copyFrame(src, core);
		VSMap* dstProps = vsapi->getFramePropertiesRW(dst);
		int height = vsapi->getFrameHeight(src, d->plane);
		int width = vsapi->getFrameWidth(src, d->plane);
		auto srcp = vsapi->getReadPtr(src, d->plane);
		ptrdiff_t stride = vsapi->getStride(src, d->plane);
		auto total = (int64_t)height * width;
		double avg = 0.0;
		uint64_t sum = 0;
		double sumf = 0.0;

		switch (d->vi->format.bytesPerSample) {
		case 1:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					auto pixel = srcp[x];
					auto result = std::find(begin(d->pxlist), end(d->pxlist), pixel);
					auto bresult = (result != std::end(d->pxlist));
					if (!bresult) {
						sum += pixel;
					}

					if (bresult) {
						total--;
					}
				}
				srcp += stride;
			}
			avg = (double)sum / (total * (((int64_t)1 << d->vi->format.bitsPerSample) - 1));
			break;
		case 2:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					auto pixel = ((const uint16_t*)srcp)[x];
					auto result = std::find(begin(d->pxlist), end(d->pxlist), pixel);
					auto bresult = (result != std::end(d->pxlist));
					if (!bresult) {
						sum += pixel;
					}

					if (bresult) {
						total--;
					}
				}
				srcp += stride;
			}
			avg = (double)sum / (total * (((int64_t)1 << d->vi->format.bitsPerSample) - 1));
			break;
		case 4:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					auto pixel = ((const float*)srcp)[x];
					auto result = std::find(begin(d->pxlist), end(d->pxlist), pixel);
					auto bresult = (result != std::end(d->pxlist));
					if (!bresult) {
						sumf += pixel;
					}

					if (bresult) {
						total--;
					}
				}
				srcp += stride;
			}
			avg = sumf / total;
			break;
		}

		vsapi->mapSetFloat(dstProps, "psmAvg", avg, maReplace);
		vsapi->freeFrame(src);
		return dst;
	}
	return nullptr;
}

static void VS_CC planeAverageFree(void* instanceData, VSCore* core, const VSAPI* vsapi) {
	auto d{ static_cast<planeAverageData*>(instanceData) };
	vsapi->freeNode(d->node);
	delete d;
}

void VS_CC planeAverageCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi) {
	auto d{ std::make_unique<planeAverageData>() };
	int err = 0;

	d->node = vsapi->mapGetNode(in, "clip", 0, nullptr);
	d->vi = vsapi->getVideoInfo(d->node);

	int numpx = vsapi->mapNumElements(in, "value_exclude");

	for (int i = 0; i < numpx; i++) {
		d->pxlist.push_back(vsapi->mapGetIntSaturated(in, "value_exclude", i, 0));
	}

	d->plane = vsapi->mapGetIntSaturated(in, "plane", 0, &err);
	if (err)
		d->plane = 0;

	if (d->plane < 0 || d->plane >= d->vi->format.numPlanes) {
		vsapi->mapSetError(out, "PlaneAverage: invalid plane specified");
		vsapi->freeNode(d->node);
		return;
	}

	VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };
	vsapi->createVideoFilter(out, "PlaneAverage", d->vi, planeAverageGetFrame, planeAverageFree, fmParallel, deps, 1, d.get(), core);
	d.release();
}