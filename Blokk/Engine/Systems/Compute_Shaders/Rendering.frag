// HLSL Fragment Shader for Maximum Performance

struct PSInput {
    float4 position : SV_POSITION;
    float2 textureCoords : TEXCOORD0;
};

// Bindings for texture and sampler
[[vk::binding(0, 1)]] Texture2D    spriteTexture : register(t0, space1);
[[vk::binding(1, 1)]] SamplerState spriteSampler : register(s1, space1);

float4 main(PSInput input) : SV_Target {
    // Single texture sample with no branching or heavy math
    return spriteTexture.Sample(spriteSampler, input.textureCoords);
}