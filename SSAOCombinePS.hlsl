struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D SceneColorsNoAmbient : register(t0);
Texture2D SSAOBlur : register(t1);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    // Sample all three
    float3 sceneColors = SceneColorsNoAmbient.Sample(BasicSampler, input.uv).rgb;
    float ao = SSAOBlur.Sample(BasicSampler, input.uv).r;
    
    // Final combine
    return pow(float4(ao * sceneColors, 1), 1.0f/2.2f);
}