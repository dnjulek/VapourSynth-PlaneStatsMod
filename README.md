> [!CAUTION] 
> DEPRECATED! Use https://github.com/dnjulek/vapoursynth-zip


Vapoursynth PlaneStats with threshold.\
Min and Max with threshold are useful for white balance scripts [like retinex](https://github.com/dnjulek/jvsfunc/blob/f7e1cad6f8cebeb011f8e6ee1d618526217da267/jvsfunc/misc.py#L53).

## PlaneMinMax

### Usage
```python
psm.PlaneMinMax(vnode clip [, float minthr=0, float maxthr=0, int plane=0])
```
### Parameters:

- clip\
    A clip to process.\
    Gets frame props named ``psmMin`` and ``psmMax``.
- minthr\
    Amount of the smallest pixels dropped before pick the PlaneStatsMin.\
    Should be a float between 0.0 and 1.0
- maxthr\
    Amount of the largest pixels dropped before pick the PlaneStatsMax.\
    Should be a float between 0.0 and 1.0
- plane\
    Plane to be used.


## PlaneAverage

### Usage
```python
psm.PlaneAverage(vnode clip, int[] value_exclude [, int plane=0, string prop='psmAvg'])
```
### Parameters:

- clip\
    A clip to process.\
    Gets frame prop named ``psmAvg``.
- value_exclude\
    List of pixel values that will be ignored during averaging.
- plane\
    Plane to be used.
- prop\
    Name of the property to be used.
