## PlaneStatsMod

Vapoursynth PlaneStats with threshold.\
Min and Max with threshold are useful for white balance scripts like retinex.

### Usage
```python
psm.PlaneStatsMod(vnode clip[, float minthr=0, float maxthr=0, int plane=0])
```
### Parameters:

- clip\
    A clip to process.
- minthr\
    Amount of the smallest pixels dropped before pick the PlaneStatsMin.\
    Should be a float between 0.0 and 1.0
- maxthr\
    Amount of the largest pixels dropped before pick the PlaneStatsMax.\
    Should be a float between 0.0 and 1.0
- plane\
    Plane to be used.