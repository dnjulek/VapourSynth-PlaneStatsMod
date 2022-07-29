#include "shared.h"

struct planeMinMaxData final {
	VSNode* node;
	float minthr;
	float maxthr;
	int plane;
};

static const VSFrame* VS_CC planeMinMaxGetFrame(int n, int activationReason, void* instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi) {
	auto d{ static_cast<planeMinMaxData*>(instanceData) };

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		const VSFrame* src = vsapi->getFrameFilter(n, d->node, frameCtx);
		VSFrame* dst = vsapi->copyFrame(src, core);

		const VSVideoFormat* fi = vsapi->getVideoFrameFormat(dst);
		VSMap* dstProps = vsapi->getFramePropertiesRW(dst);
		int height = vsapi->getFrameHeight(src, d->plane);
		int width = vsapi->getFrameWidth(src, d->plane);
		const uint8_t* srcp = vsapi->getReadPtr(src, d->plane);
		ptrdiff_t stride = vsapi->getStride(src, d->plane);

		int pixelsize = fi->bytesPerSample;
		const int bits_per_pixel = fi->bitsPerSample;
		const int max_pixel_value = (1 << bits_per_pixel) - 1;
		const int buffersize = pixelsize == 4 ? 65536 : (1 << bits_per_pixel);
		uint32_t* accum_buf = new uint32_t[buffersize];
		std::fill_n(accum_buf, buffersize, 0);

		switch (pixelsize) {
		case 1:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					accum_buf[srcp[x]]++;
				}
				srcp += stride;
			}
			break;
		case 2:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					accum_buf[std::min((int)(reinterpret_cast<const uint16_t*>(srcp)[x]), max_pixel_value)]++;
				}
				srcp += stride;
			}
			break;
		case 4:
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					const float pixel = reinterpret_cast<const float*>(srcp)[x];
					accum_buf[std::clamp((int)(65535.0f * pixel + 0.5f), 0, 65535)]++;
				}
				srcp += stride;
			}
			break;
		}


		int retvalmin = buffersize - 1;
		unsigned int tpixelsmin = (unsigned int)(height * width * d->minthr);
		unsigned int countmin = 0;

		for (int i = 0; i < buffersize; i++) {
			countmin += accum_buf[i];
			if (countmin > tpixelsmin) {
				retvalmin = i;
				break;
			}
		}

		if (pixelsize == 4) {
			double retvalminf = retvalmin / 65535.0;
			vsapi->mapSetFloat(dstProps, "psmMin", retvalminf, maReplace);
		}
		else {
			vsapi->mapSetInt(dstProps, "psmMin", retvalmin, maReplace);
		}


		int retvalmax = 0;
		unsigned int tpixelsmax = (unsigned int)(height * width * d->maxthr);
		unsigned int countmax = 0;

		for (int i = buffersize - 1; i >= 0; i--) {
			countmax += accum_buf[i];
			if (countmax > tpixelsmax) {
				retvalmax = i;
				break;
			}
		}

		if (pixelsize == 4) {
			double retvalmaxf = retvalmax / 65535.0;
			vsapi->mapSetFloat(dstProps, "psmMax", retvalmaxf, maReplace);
		}
		else {
			vsapi->mapSetInt(dstProps, "psmMax", retvalmax, maReplace);
		}

		delete[] accum_buf;

		vsapi->freeFrame(src);
		return dst;
	}
	return nullptr;
}

static void VS_CC planeMinMaxFree(void* instanceData, VSCore* core, const VSAPI* vsapi) {
	auto d{ static_cast<planeMinMaxData*>(instanceData) };
	vsapi->freeNode(d->node);
	delete d;
}

void VS_CC planeMinMaxCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi) {
	auto d{ std::make_unique<planeMinMaxData>() };
	int err = 0;

	d->node = vsapi->mapGetNode(in, "clip", 0, nullptr);
	const VSVideoInfo* vi = vsapi->getVideoInfo(d->node);

	d->minthr = vsapi->mapGetFloatSaturated(in, "minthr", 0, &err);
	if (err)
		d->minthr = 0.0f;

	d->maxthr = vsapi->mapGetFloatSaturated(in, "maxthr", 0, &err);
	if (err)
		d->maxthr = 0.0f;

	d->plane = vsapi->mapGetIntSaturated(in, "plane", 0, &err);
	if (err)
		d->plane = 0;

	if (d->plane < 0 || d->plane >= vi->format.numPlanes) {
		vsapi->mapSetError(out, "PlaneMinMax: invalid plane specified");
		vsapi->freeNode(d->node);
		return;
	}

	if (d->minthr < 0 || d->minthr > 1) {
		vsapi->mapSetError(out, "PlaneMinMax: minthr should be a float between 0.0 and 1.0");
		vsapi->freeNode(d->node);
		return;
	}

	if (d->maxthr < 0 || d->maxthr > 1) {
		vsapi->mapSetError(out, "PlaneMinMax: maxthr should be a float between 0.0 and 1.0");
		vsapi->freeNode(d->node);
		return;
	}

	VSFilterDependency deps[] = { {d->node, rpStrictSpatial} };
	vsapi->createVideoFilter(out, "PlaneMinMax", vi, planeMinMaxGetFrame, planeMinMaxFree, fmParallel, deps, 1, d.get(), core);
	d.release();
}