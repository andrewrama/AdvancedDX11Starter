cbuffer externalData : register(b0)
{
    matrix view;
    matrix projection;    
    float4 startColor;
    float4 endColor;
    float3 accel;
    float particleLifetime;
    float startSize;
    float endSize;
    float startAlpha;
    float endAlpha;
    float currentTime;
};
struct Particle
{
    float EmitTime;
    float3 StartPosition;
    float3 StartVelocity;
};
// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 colorTint : COLOR;
};

VertexToPixel main(uint id : SV_VertexID)
{
    // Set up output
    VertexToPixel output;
    // Get id info
    uint particleID = id / 4; // Every 4 verts are ONE particle!
    uint cornerID = id % 4; // 0,1,2,3 = which corner of the "quad"
    // Grab one particle
    Particle p = ParticleData.Load(particleID);
    
    float age = currentTime - p.EmitTime;
    float agePercentage = age / particleLifetime;
    
    float size = lerp(startSize, endSize, agePercentage);
    float alpha = lerp(startAlpha, endAlpha, agePercentage);
    
    // Offsets for the 4 corners of a quad - we'll only use one for each
    // vertex, but which one depends on the cornerID
    // Scale by size
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f) * size; // TL
    offsets[1] = float2(+1.0f, +1.0f) * size; // TR
    offsets[2] = float2(+1.0f, -1.0f) * size; // BR
    offsets[3] = float2(-1.0f, -1.0f) * size; // BL
    
    // Billboarding!
    // Offset the position based on the camera's right and up vectors
    
    float3 pos = accel * age * age / 2.0f + p.StartVelocity * age + p.StartPosition;
    pos += float3(view._11, view._12, view._13) * offsets[cornerID].x; // RIGHT
    pos += float3(view._21, view._22, view._23) * offsets[cornerID].y; // UP
    
    matrix viewProj = mul(projection, view);
    output.position = mul(viewProj, float4(pos, 1.0f));
    
    float2 uvs[4];
    uvs[0] = float2(0, 0); //TL
    uvs[1] = float2(1, 0); //TR
    uvs[2] = float2(1, 1); //BR
    uvs[3] = float2(0, 1); //BL
    
    output.uv = uvs[cornerID];
    output.colorTint = lerp(startColor, endColor, agePercentage);
    
    // Update alpha value separately
    output.colorTint.a = alpha;

    return output;
}