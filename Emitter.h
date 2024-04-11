#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <memory>
#include "Camera.h"
#include "Material.h"


struct Particle 
{
	float EmitTime;
	DirectX::XMFLOAT3 StartPos;
};

class Emitter
{
private:
	// Particle Data
	int maxParticles;
	Particle* particles;
	int indexFirstDead;
	int indexFirstAlive;
	int livingParticleCount;

	DirectX::XMFLOAT3 direction;

	// Emission Properties
	int particlesPerSecond;
	float secondsBetweenEmission;
	float timeSinceLastEmit;

	// Rendering Resources
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	Transform* transform;
	std::shared_ptr<Material> material;

	//Methods
	void EmitParticle(float currentTime);
	void UpdateSingleParticle(float currentTime, int index);
	void CreateParticlesAndGPUResources();
	void CopyParticlesToGPU(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

public:
	Emitter(Microsoft::WRL::ComPtr<ID3D11Device> device, 
		std::shared_ptr<Material> material,
		int maxParticles,
		int particlesPerSecond,
		float maxParticleLifetime,
		DirectX::XMFLOAT3 startPos = DirectX::XMFLOAT3(0, 0, 0),
		DirectX::XMFLOAT3 direction = DirectX::XMFLOAT3(1, 0, 0));

	~Emitter();

	void Update(float deltaTime, float currentTime);
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float currentTime);

	float maxParticleLifetime;
	int GetMaxParticles();
	int GetParticlesPerSecond();

	Transform* GetTransform();
	std::shared_ptr<Material> GetMaterial();
	void SetMaterial(std::shared_ptr<Material> material);
};

