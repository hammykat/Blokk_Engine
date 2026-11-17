// This file uses a swap-and-delete approach to delete many objects at different indexes

struct ConstantFrameData {
    uint ObjectCount;
    uint NumberOfIndexesToDelete;
    uint NumberOfObjectsSpawned; 
    float2 CameraPosition;
    float2 ScreenDimensions;
    float DeltaTime;
};


// Push Constants
[[vk::push_constants]] ConstantFrameData FrameConstants;

// Buffers mapped to contiguous layout (0 to 11)
[[vk::binding(0, 0)]] RWStructuredBuffer<float2> Velocities : register(t0, space0); // Object velocities
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(u1, space0); // Object positions [Updated by: GPU & CPU]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4, space0); // Frame numbers [Updated by: GPU & CPU]
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> AnimationNumbers : register(u5, space0); // Object animation index in memory [Updated by: CPU]

// Deletion Buffer (Now at binding 10)
[[vk::binding(10, 0)]] StructuredBuffer<uint> IndexesToDelete : register(t10, space0); // Indexes the user wants to delete

void SwapIndexes(uint indexA, uint indexB)
{
	// Swap velocities
	float2 tempVel = Velocities[indexA];
	Velocities[indexA] = Velocities[indexB];
	Velocities[indexB] = tempVel;
    
	// Swap positions
	float2 tempPos = Positions[indexA];
	Positions[indexA] = Positions[indexB];
	Positions[indexB] = tempPos;
    
	// Swap frame numbers
	uint tempFrameNum = FrameNumbers[indexA];
	FrameNumbers[indexA] = FrameNumbers[indexB];
	FrameNumbers[indexB] = tempFrameNum;
    
	// Swap animation numbers
	uint tempAnimNum = AnimationNumbers[indexA];
	AnimationNumbers[indexA] = AnimationNumbers[indexB];
	AnimationNumbers[indexB] = tempAnimNum;
}

[numthreads(256, 1, 1)]
void main(uint3 _id : SV_DispatchThreadID)
{
	uint id = _id.x;

	if(id >= FrameConstants.NumberOfIndexesToDelete) return;

	uint startingIndexToSwap = FrameConstants.ObjectCount - FrameConstants.NumberOfIndexesToDelete;

	uint indexA = IndexesToDelete[id];
	uint indexB = FrameConstants.ObjectCount - id - 1;		

	if(indexA != indexB)
		SwapIndexes(indexA, indexB);
}