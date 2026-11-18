// This file finds new objects' animation indexes in the arrays using their animation index and their frame number

struct ConstantFrameData {
    uint ObjectCount;
    uint NumberOfIndexesToDelete;
    uint NumberOfObjectsSpawned; 
    float2 CameraPosition;
    float2 ScreenDimensions;
    float DeltaTime;
};

[[vk::push_constant]] ConstantFrameData FrameConstants;
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2, space0); // Number of frames in animations [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4, space0); // Frame numbers [Updated by: GPU & CPU]
[[vk::binding(5, 0)]] StructuredBuffer<uint> AnimationNumbers : register(t5, space0); // Object animation index in memory [Updated by: CPU]
[[vk::binding(6, 0)]] StructuredBuffer<uint> AnimationStarts : register(t6, space0); // Animation start index in memory [Updated by: CPU]

[numthreads(256, 1, 1)]
void main(uint3 _id : SV_DispatchThreadID) {
  uint id = _id.x;  
	
  if(id >= FrameConstants.NumberOfObjectsSpawned) return;

  uint offset = FrameConstants.ObjectCount - FrameConstants.NumberOfObjectsSpawned;
  uint idx = id + offset;

  uint newIndex = AnimationStarts[AnimationNumbers[idx]] + FrameNumbers[idx];
  FrameNumbers[idx] = newIndex;
}
