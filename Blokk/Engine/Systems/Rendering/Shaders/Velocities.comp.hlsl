
// This shader increments object frames and updated positions (using velocities and delta)

struct ConstantFrameData {
    uint ObjectCount;
    uint NumberOfIndexesToDelete;
    uint NumberOfObjectsSpawned; 
    float2 CameraPosition;
    float2 ScreenDimensions;
    float DeltaTime;
};

[[vk::push_constant]] ConstantFrameData FrameConstants;
[[vk::binding(0, 0)]] StructuredBuffer<float2> Velocities : register(t0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(u1); // Object positions [Updated by: GPU & CPU]
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2); // Number of frames in animations [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4); // Frame numbers [Updated by: GPU & CPU]

[numthreads(256, 1, 1)]
void main(uint3 _id : SV_DispatchThreadID) {
    uint id = _id.x;

    // Only update the right objects
    if(id >= FrameConstants.ObjectCount) return;

    // Update a frame number
    FrameNumbers[id] = (FrameNumbers[id] + 1) % AnimationSizes[id];

    // Update a position
    Positions[id] += Velocities[id] * FrameConstants.DeltaTime;
}


