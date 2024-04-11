cbuffer externalData : register(b0)
{
    matrix view;
    matrix projection;
    float currentTime;
    float3 direction;
    float particleLifetime;
};
struct Particle
{
    float EmitTime;
    float3 StartPosition;
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
    
    // Offsets for the 4 corners of a quad - we'll only use one for each
    // vertex, but which one depends on the cornerID
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f); // TL
    offsets[1] = float2(+1.0f, +1.0f); // TR
    offsets[2] = float2(+1.0f, -1.0f); // BR
    offsets[3] = float2(-1.0f, -1.0f); // BL
    
    // Billboarding!
    // Offset the position based on the camera's right and up vectors
    float3 pos = lerp(p.StartPosition, direction, age);
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
    
    return output;
}