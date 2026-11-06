// HLSL Vertex Shader for GPU-Driven Indirect Rendering

struct VSInput {
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID; // Maps to the instance index processed by vkCmdDrawIndirect
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 textureCoords : TEXCOORD0;
};

// Bindings matching your descriptor layout in space0
[[vk::binding(0, 0)]] StructuredBuffer<int> XPositions   : register(t0, space0);
[[vk::binding(1, 0)]] StructuredBuffer<int> YPositions   : register(t1, space0);
[[vk::binding(2, 0)]] StructuredBuffer<int> Widths       : register(t2, space0);
[[vk::binding(3, 0)]] StructuredBuffer<int> Heights      : register(t3, space0);
[[vk::binding(8, 0)]] StructuredBuffer<uint> VisibleIndexes : register(t8, space0);

cbuffer ScreenDimensions : register(b5, space0) {
    int scrWidth;
    int scrHeight;
};

// Quad vertices matching your layout (2 triangles / 4 unique vertices or 6)
static const float2 QUAD_VERTICES[4] = {
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f)
};

VSOutput main(VSInput input) {
    // 1. Look up the real object ID from the compacted visible list using the instance ID
    uint realObjectID = VisibleIndexes[input.instanceID];
    
    // 2. Grab texture coordinates
    VSOutput output;
    output.textureCoords = QUAD_VERTICES[input.vertexID];

    // 3. Fetch position and size from SoA buffers using the real object ID
    float2 screenPixelPos = float2(XPositions[realObjectID], YPositions[realObjectID]);
    float2 objectSize = float2(Widths[realObjectID], Heights[realObjectID]);

    // 4. Compute final pixel position
    float2 finalPixelPos = screenPixelPos + (output.textureCoords * objectSize);

    // 5. Convert pixels to NDC space (-1 to 1) with flipped Y for Vulkan
    float2 ndcPos = (finalPixelPos / float2(scrWidth, scrHeight)) * 2.0f - 1.0f;
    ndcPos.y = -ndcPos.y;

    output.position = float4(ndcPos, 0.0f, 1.0f);
    return output;
}