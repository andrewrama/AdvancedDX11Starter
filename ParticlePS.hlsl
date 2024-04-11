struct VertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float4 colorTint : COLOR;
};

Texture2D Particle : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    return Particle.Sample(BasicSampler, input.uv) * input.colorTint;
}