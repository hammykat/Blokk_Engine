// Contains layout bindings - Data in all compute shaders
// File not to be used or compiled, only as a reference
// Store number of objects in each category

struct ConstantFrameData {
    uint ObjectCount;
    float2 CameraPosition;
    float2 ScreenDimensions;
    float DeltaTime;
};

// Ownership tags ---------------------------------------
// * [Updated by: CPU]:
//   The GPU DOES NOT UPDATE that buffer. Instead, the CPU updates that data and sends it to the GPU.
// * [Updated by: GPU]:
//   The GPU OWNS THE MEMORY COMPLETELY. This means that the CPU can't easily acess the data or 
//   update it very fast. Most data is updated by GPU (CPU <-> GPU overhead and data transfer is slow).
// * [Updated by: CPU & GPU]:
//   While the GPU owns and updates the data, the CPU can still update it. For example, positions can 
//   be updated byt the CPU (The user can set the object's position) BUT is also handled by the GPU (updating using velocities).
// * [NOT UPDATED]:
// * The CPU has the data and that data DOES NOT CHANGE after the start. The data is only sent from the CPU -> GPU at 
//   start and then not touched by either. For example, animation data is NOT ABLE TO BE CHANGED AFTER THE START (like 
// animations can't be created, added to, or removed), so the GPU only reads the memory and it is not updated by either of them.
// ----------------------------------------------------------

// push_constants optimized for speed when CPU updates (Maximum 120 bytes!)
[[vk::push_constants]] FrameConstantsData : FrameData;

// Positions
[[vk::binding(0, 0)]] RWStructuredBuffer<float2> Velocities : register(t0, space0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(u1, space0); // Object positions [Updated by: GPU & CPU]

// Animations
[[vk::binding(2, 0)]] StructuredBuffer<uint> AnimationSizes : register(t2, space0); // Number of frames in animations [NOT UPDATED]
[[vk::binding(3, 0)]] StructuredBuffer<float2> FrameDimensions : register(t3, space0); // Frame height and widths [NOT UPDATED]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4, space0); // Frame numbers [Updated by: GPU & CPU]
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> AnimationNumbers : register(t5, space0); // Object animation index in memory [Updated by: CPU]
[[vk::binding(6, 0)]] StructuredBuffer<uint> AnimationStarts : register(t6, space0); // Animation start index in memory [Updated by: CPU]

[[vk::binding(7, 0)]] StructuredBuffer<bool> IsVisible : register(t7, space0); // User set to visible? [Updated by: CPU]

// Outputs
[[vk::binding(8, 0)]] RWStructuredBuffer<uint> VisibleIndexes : register(u8, space0); // Compacted list of object IDs to render
[[vk::binding(9, 0)]] RWByteAddressBuffer IndirectArgs : register(u9, space0); // VkDrawIndirectCommand structure packed on GPU
[[vk::binding(10, 0)]] RWStructuredBuffer<float2> RenderPositions : register(u10, space0); // Holds positions offsetted by camera

// Variables for the deletion of objects. The CPU can request to delete objects and the GPU will handle it. The GPU will also update the number of active objects to render.
[[vk::binding(11, 0)]] RWBuffer<uint> NumberOfActiveObjects : register(u11, space0); // Number of active objects to render [Updated by: GPU]
[[vk::binding(12, 0)]] RWBuffer<uint> NumberOfIndexesToDelete : register(u12, space0); // Number of indexes the user wants to delete
[[vk::binding(13, 0)]] RWStructuredBuffer<uint> IndexesToDelete : register(u13, space0); // Indexes the user wants to delete

