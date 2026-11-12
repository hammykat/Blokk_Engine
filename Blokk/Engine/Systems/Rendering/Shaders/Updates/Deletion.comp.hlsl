
[[vk::binding(11, 0)]] RWBuffer<uint> NumberOfActiveObjects : register(u11, space0); // Number of active objects to render [Updated by: GPU]
[[vk::binding(12, 0)]] RWBuffer<uint> NumberOfIndexesToDelete : register(u12, space0); // Number of indexes the user wants to delete
[[vk::binding(13, 0)]] RWStructuredBuffer<uint> IndexesToDelete : register(u13, space0); // Indexes the user wants to delete

// Buffers it needs to remove the index from
[[vk::binding(0, 0)]] RWStructuredBuffer<float2> Velocities : register(u0, space0); // Object velocities [Updated by: CPU]
[[vk::binding(1, 0)]] RWStructuredBuffer<float2> Positions : register(u1, space0); // Object positions [Updated by: GPU & CPU]
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> FrameNumbers : register(u4, space0); // Frame numbers [Updated by: GPU & CPU]
[[vk::binding(5, 0)]] RWStructuredBuffer<uint> AnimationNumbers : register(u5, space0); // Object animation index in memory [Updated by: CPU]


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

	if(id >= NumberOfIndexesToDelete[0]) return;

	uint startingIndexToSwap = NumberOfActiveObjects[0] - NumberOfIndexesToDelete[0];

	uint indexA = IndexesToDelete[id];
	uint indexB = NumberOfActiveObjects[0] - id - 1;		

	if(indexA != indexB)
		SwapIndexes(indexA, indexB);
}

