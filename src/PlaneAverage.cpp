#include "shared.h"

struct planeAverageData final {
	VSNode* node;
	const VSVideoInfo* vi;
	std::vector<int> pxlist;
	int plane;
};

template<typename pixel_t>
static float process_c(const VSFrame* src, VSFrame* dst, const planeAverageData* const VS_RESTRICT d, const VSAPI* vsapi) noexcept {
	const auto width{ vsapi->getFrameWidth(src, d->plane) };
	const auto height{ vsapi->getFrameHeight(src, d->plane) };
	const auto stride{ vsapi->getStride(src, d->plane) / d->vi->format.bytesPerSample };
	auto srcp{ reinterpret_cast<const pixel_t*>(vsapi->getReadPtr(src, d->plane)) };

	auto total = static_cast<int64_t>(height * width);
	typedef typename std::conditional < sizeof(pixel_t) == 4, double, int64_t>::type sum_t;
	sum_t sum{ 0 };

	for (int y{ 0 }; y < height; y++) {
		for (int x{ 0 }; x < width; x++) {
			auto pixel{ srcp[x] };
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

	if (total == 0)
		return 0.0f;
	else if constexpr (std::is_integral_v<pixel_t>)
		return (static_cast<float>(sum) / total) / ((1 << d->vi->format.bitsPerSample) - 1);
	else
		return static_cast<float>(sum / total);
}


static const VSFrame* VS_CC planeAverageGetFrame(int n, int activationReason, void* instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi) {
	auto d{ static_cast<planeAverageData*>(instanceData) };

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		const VSFrame* src = vsapi->getFrameFilter(n, d->node, frameCtx);
		VSFrame* dst = vsapi->copyFrame(src, core);
		VSMap* dstProps = vsapi->getFramePropertiesRW(dst);
		float avg{ 0.0f };

		switch (d->vi->format.bytesPerSample) {
		case 1:
			avg = process_c<uint8_t>(src, dst, d, vsapi);
			break;
		case 2:
			avg = process_c<uint16_t>(src, dst, d, vsapi);
			break;
		case 4:
			avg = process_c<float>(src, dst, d, vsapi);
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