#include "Emitter.h"

Emitter::Emitter(Microsoft::WRL::ComPtr<ID3D11Device> device,
    std::shared_ptr<Material> material,
    int maxParticles,
    int particlesPerSecond,
    float maxParticleLifetime,
    DirectX::XMFLOAT3 startPos,
    DirectX::XMFLOAT3 direction,
    float startSize,
    float endSize,
    DirectX::XMFLOAT4 startColor,
    DirectX::XMFLOAT4 endColor)
    :
    device(device),
    material(material),
    maxParticles(maxParticles),
    particlesPerSecond(particlesPerSecond),
    maxParticleLifetime(maxParticleLifetime),
    direction(direction),
    startSize(startSize),
    endSize(endSize),
    startColor(startColor),
    endColor(endColor)
{
    secondsBetweenEmission = 1.0f / particlesPerSecond;

    timeSinceLastEmit = 0.0f;
    livingParticleCount = 0;
    indexFirstAlive = 0;
    indexFirstDead = 0;
    particles = new Particle[maxParticles];
    transform = new Transform();

    transform->SetPosition(startPos);

    CreateParticlesAndGPUResources();
}

Emitter::~Emitter()
{
    delete[] particles;
    delete transform;
}

void Emitter::EmitParticle(float currentTime)
{
    if (livingParticleCount == maxParticles)
        return;

    particles[indexFirstDead].EmitTime = currentTime;

    particles[indexFirstDead].StartPos = transform->GetPosition();
    particles[indexFirstDead].StartPos.x += ((float)rand() / RAND_MAX * (1.0f - -1.0f) + -1.0f);
    particles[indexFirstDead].StartPos.y += ((float)rand() / RAND_MAX * (1.0f - -1.0f) + -1.0f);
    particles[indexFirstDead].StartPos.z += ((float)rand() / RAND_MAX * (1.0f - -1.0f) + -1.0f);

    indexFirstDead++;
    indexFirstDead %= maxParticles;

    livingParticleCount++;
}

void Emitter::UpdateSingleParticle(float currentTime, int index)
{
    float age = currentTime - particles[index].EmitTime;

    // Update and check for death
    if (age >= maxParticleLifetime)
    {
        // Recent death, so retire by moving alive count (and wrap)
        indexFirstAlive++;
        indexFirstAlive %= maxParticles;
        livingParticleCount--;
    }
}

void Emitter::CreateParticlesAndGPUResources()
{
    if (particles) delete[] particles;
    indexBuffer.Reset();
    particleBuffer.Reset();
    particleSRV.Reset();

    particles = new Particle[maxParticles];
    ZeroMemory(particles, sizeof(Particle) * maxParticles);

    unsigned int* indices = new unsigned int[maxParticles * 6];
    int indexCount = 0;
    for (int i = 0; i < maxParticles * 4; i += 4)
    {
        indices[indexCount++] = i;
        indices[indexCount++] = i + 1;
        indices[indexCount++] = i + 2;
        indices[indexCount++] = i;
        indices[indexCount++] = i + 2;
        indices[indexCount++] = i + 3;
    }
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
    device->CreateBuffer(&ibDesc, &indexData, indexBuffer.GetAddressOf());
    delete[] indices;

    D3D11_BUFFER_DESC allParticleBufferDesc = {};
    allParticleBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    allParticleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    allParticleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    allParticleBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    allParticleBufferDesc.StructureByteStride = sizeof(Particle);
    allParticleBufferDesc.ByteWidth = sizeof(Particle) * maxParticles;
    device->CreateBuffer(&allParticleBufferDesc, 0, particleBuffer.GetAddressOf());

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = maxParticles;
    device->CreateShaderResourceView(particleBuffer.Get(), &srvDesc, particleSRV.GetAddressOf());
}

void Emitter::CopyParticlesToGPU(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
    // Map the buffer, locking it on the GPU so we can write to it
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map(particleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    // How are living particles arranged in the buffer?
    if (indexFirstAlive < indexFirstDead)
    {
        // Only copy from FirstAlive -> FirstDead
        memcpy(
            mapped.pData, // Destination = start of particle buffer
            particles + indexFirstAlive, // Source = particle array, offset to first living particle
            sizeof(Particle) * livingParticleCount); // Amount = number of particles (measured in BYTES!)
    }
    else
    {
        // Copy from 0 -> FirstDead
        memcpy(
            mapped.pData, // Destination = start of particle buffer
            particles, // Source = start of particle array
            sizeof(Particle) * indexFirstDead); // Amount = particles up to first dead (measured in BYTES!)
        // ALSO copy from FirstAlive -> End
        memcpy(
            (void*)((Particle*)mapped.pData + indexFirstDead), // Destination = particle buffer, AFTER the data we copied in previous memcpy()
            particles + indexFirstAlive, // Source = particle array, offset to first living particle
            sizeof(Particle) * (maxParticles - indexFirstAlive)); // Amount = number of living particles at end of array (measured in BYTES!)
    }
    context->Unmap(particleBuffer.Get(), 0);
}

void Emitter::Update(float deltaTime, float currentTime)
{
    // Anything to update?
    if (livingParticleCount > 0)
    {
        // Update all particles - Check cyclic buffer first
        if (indexFirstAlive < indexFirstDead)
        {
            // First alive is BEFORE first dead, so the "living" particles are contiguous
            // 
            // 0 -------- FIRST ALIVE ----------- FIRST DEAD -------- MAX
            // |    dead    |            alive       |         dead    |

            // First alive is before first dead, so no wrapping
            for (int i = indexFirstAlive; i < indexFirstDead; i++)
                UpdateSingleParticle(currentTime, i);
        }
        else if (indexFirstDead < indexFirstAlive)
        {
            // First alive is AFTER first dead, so the "living" particles wrap around
            // 
            // 0 -------- FIRST DEAD ----------- FIRST ALIVE -------- MAX
            // |    alive    |            dead       |         alive   |

            // Update first half (from firstAlive to max particles)
            for (int i = indexFirstAlive; i < maxParticles; i++)
                UpdateSingleParticle(currentTime, i);

            // Update second half (from 0 to first dead)
            for (int i = 0; i < indexFirstDead; i++)
                UpdateSingleParticle(currentTime, i);
        }
        else
        {
            // First alive is EQUAL TO first dead, so they're either all alive or all dead
            // - Since we know at least one is alive, they should all be
            //
            //            FIRST ALIVE
            // 0 -------- FIRST DEAD -------------------------------- MAX
            // |    alive     |                   alive                |
            for (int i = 0; i < maxParticles; i++)
                UpdateSingleParticle(currentTime, i);
        }
    }

    timeSinceLastEmit += deltaTime;
    while (timeSinceLastEmit > secondsBetweenEmission)
    {
        EmitParticle(currentTime);
        timeSinceLastEmit = 0;
    }

}

void Emitter::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float currentTime)
{
    CopyParticlesToGPU(context);

    UINT stride = 0;
    UINT offset = 0;
    ID3D11Buffer* nullBuffer = 0;
    context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    material->PrepareMaterial(transform, camera);

    std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
    vs->SetMatrix4x4("view", camera->GetView());
    vs->SetMatrix4x4("projection", camera->GetProjection());
    vs->SetFloat("currentTime", currentTime);
    vs->SetFloat3("direction", direction);
    vs->SetFloat("particleLifetime", maxParticleLifetime);
    vs->SetFloat4("startColor", startColor);
    vs->SetFloat4("endColor", endColor);
    vs->SetFloat("startSize", startSize);
    vs->SetFloat("endSize", endSize);
    vs->SetShaderResourceView("ParticleData", particleSRV);
    vs->CopyAllBufferData();

    std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
    ps->SetShaderResourceView("Particle", material->GetTextureSRV("Particle"));

    context->DrawIndexed(livingParticleCount * 6, 0, 0);
}

int Emitter::GetMaxParticles()
{
    return maxParticles;
}

int Emitter::GetParticlesPerSecond()
{
    return particlesPerSecond;
}

Transform* Emitter::GetTransform()
{
    return transform;
}

std::shared_ptr<Material> Emitter::GetMaterial()
{
    return material;
}

void Emitter::SetMaterial(std::shared_ptr<Material> material)
{
    this->material = material;
}
