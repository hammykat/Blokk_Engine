// This shader handles visibility culling and saves the result

// Store number of objects in each category
struct ConstantFrameData {
    uint ObjectCount;
    uint NumberOfIndexesToDelete;
    uint NumberOfObjectsSpawned; 
    float2 CameraPosition;
    float2 ScreenDimensions;
    float DeltaTime;
};

// Constant Buffer / Push Constants
[[vk::push_constants]] ConstantFrameData FrameData;

// Positions
[[vk::binding(0, 0)]] StructuredBuffer<float2> Velocities : register(t0, space0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(u1, space0); // Object positions [Updated by: GPU & CPU]

// Animations
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2, space0); // Number of frames in animations [NOT UPDATED]
[[vk::binding(3, 0)]] StructuredBuffer<float2> FrameDimensions : register(t3, space0); // Frame height and widths [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4, space0); // Frame numbers [Updated by: GPU & CPU]
[[vk::binding(5, 0)]] StructuredBuffer<uint> AnimationNumbers : register(t5, space0); // Object animation index in memory [Updated by: CPU]
[[vk::binding(6, 0)]] StructuredBuffer<uint> AnimationStarts : register(t6, space0); // Animation start index in memory [Updated by: CPU]

// Outputs & Rendering
[[vk::binding(8, 0)]] RWStructuredBuffer<uint> VisibleIndexes : register(u8, space0); // Compacted list of object IDs to render
[[vk::binding(9, 0)]] RWByteAddressBuffer IndirectArgs : register(u9, space0); // VkDrawIndirectCommand structure packed on GPU
[[vk::binding(10, 0)]] RWStructuredBuffer<float2> RenderPositions : register(u10, space0); // Holds positions offsetted by camera


[numthreads(256, 1, 1)]
void main(uint3 _id : SV_DispatchThreadID) {
  uint id = _id.x;  
  
  // Make sure not to try to get an invalid item
  if (id >= (uint)FrameData.ObjectCount)
    return;  

  // Get the proper width & height
  uint AnimationStart = AnimationStarts[AnimationNumbers[id]];
  float2 FrameDimension = FrameDimensions[AnimationStart + FrameNumbers[id]];  
  
  // Apply camera offsetting and save to render position array
  RenderPositions[id] = Positions[id] - FrameData.CameraPosition;  
  
  // Cache
  int2 pos = int2(RenderPositions[id]); 
  float2 ScrDimensions = FrameData.ScreenDimensions;  
  
  // Check y bounds
  bool inYBounds = (pos.y < (int)ScrDimensions.y) && 
    ((pos.y + (int)FrameDimension.y) > 0);  
    
  // Don't waste time computing x bounds
  if (!inYBounds)
    return;  
    
  // Check x bounds
  bool inXBounds = (pos.x < (int)ScrDimensions.x) && 
    ((pos.x + (int)FrameDimension.x) > 0);  
    
  // Save result
  if(inXBounds) {
    uint renderIndex;
    IndirectArgs.InterlockedAdd(4, 1, renderIndex);
    VisibleIndexes[renderIndex] = id;
  }
}