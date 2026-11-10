# Engine Layout

* [GPU Buffer Layout](Systems/Shaders/Layout_Bindings.hlsl)
* [Velocities](#velocities)
* [Incrementing Frames](#incrementing-frames)

## Velocities

> Updating object positions using their velocities
* [CPU Updates](#cpu-updates)
* [GPU Updates](#gpu-updates)

### CPU Updates:

* Systems/Velocities/Velocities-inl.h 
* Systems/Velocities/Velocities.cpp

**`Velocities-inl.h`**

Contains the actual function that updates a range of velocities using SIMD, `ProcessVelocities_SIMD`.
Templated function:
```cpp
template <class Arch>
```
Function parameters:
```cpp
float* PosX,
float* PosY,
const float* VelX,
const float* VelY,
const uint32_t Size,
const double delta
```

**`Velocities.cpp`**

Contains the `EngineCore` member function that calls the SIMD function and passes the member variables to the function, `UpdateRangeOfPositionsFn`.
Templated function:
```cpp
template <class Arch>
```
Function parameters:
```cpp
Worker& worker, 
IndexRange& Range
```

### GPU Updates

* [Systems/Rendering/Shaders/Velocities/Velocities.comp.hlsl](Systems/Rendering/Shaders/Velocities/Velocities.comp.hlsl)
* [Systems/Rendering/Shaders/Velocities/Velocities.comp.spv](Systems/Rendering/Shaders/Velocities/Velocities.comp.spv)

**`Velocities.comp.hlsl`**

Contains the shader that updates the velocities. 
Uses `[numthreads(256, 1, 1)]`. Buffers:
```hlsl
[[vk::push_constant]] ConstantFrameData FrameData;
[[vk::binding(0, 0)]] StructuredBuffer<float2> Velocities : register(t0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(t1); // Object positions [Updated by: GPU & CPU]
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2); // Number of frames in animations [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(t4); // Frame numbers [Updated by: GPU & CPU]
```

## Incrementing Frames

> Incrementing object's animation frame numbers 
> ensures they don't overflow past the animation size
* [CPU Updates](#cpu-updates-1)
* [GPU Updates](#gpu-updates-1)

### CPU Updates

* [Systems/IncrementFrames/IncrementFrames-inl.h](Systems/IncrementFrames/IncrementFrames-inl.h)
* [Systems/IncrementFrames/IncremntFrames.cpp](Systems/IncrementFrames/IncremntFrames.cpp)

**`IncrementFrames-inl.h`**

Contains the SIMD function that can update a range of frame numbers, `IncrementFramesImpl`.
Templated function
```cpp
template <class Arch>
```
Function parameters:
```cpp
uint32_t* FrameNumbers,
uint32_t Size
```

### GPU Updates

* [Systems/Rendering/Shaders/Velocities/Velocities.comp.hlsl](Systems/Rendering/Shaders/Velocities/Velocities.comp.hlsl)
* [Systems/Rendering/Shaders/Velocities/Velocities.comp.spv](Systems/Rendering/Shaders/Velocities/Velocities.comp.spv)

**`Velocities.comp.hlsl`**

Contains the shader that increments the frames. 
Uses `[numthreads(256, 1, 1)]`. Buffers:
```hlsl
[[vk::push_constant]] ConstantFrameData FrameData;
[[vk::binding(0, 0)]] StructuredBuffer<float2> Velocities : register(t0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(t1); // Object positions [Updated by: GPU & CPU]
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2); // Number of frames in animations [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(t4); // Frame numbers [Updated by: GPU & CPU]
```



