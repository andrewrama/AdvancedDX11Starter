#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "Sky.h"
#include "Emitter.h"

#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

enum RenderTargetType
{
	SCENE_COLOR_DIRECT,
	SCENE_COLOR_INDIRECT,
	SCENE_NORMALS,
	SCENE_DEPTHS,
	SSAO_RESULTS,
	SSAO_BLUR,

	// Count is always the last one!
	RENDER_TARGET_TYPE_COUNT
};

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Our scene
	std::vector<std::shared_ptr<GameEntity>> entities;
	std::shared_ptr<Camera> camera;

	// Lights
	std::vector<Light> lights;
	int lightCount;
	bool showPointLights;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendState;
	std::vector<std::shared_ptr<Emitter>> emitters;

	// These will be loaded along with other assets and
	// saved to these variables for ease of access
	std::shared_ptr<Mesh> lightMesh;
	std::shared_ptr<SimpleVertexShader> lightVS;
	std::shared_ptr<SimplePixelShader> lightPS;

	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;

	// Skybox
	std::shared_ptr<Sky> sky;

	// General helpers for setup and drawing
	void LoadAssetsAndCreateEntities();
	void GenerateLights();
	void DrawPointLights();
	void PostProcess();

	void CreateRenderTarget(
		unsigned int width,
		unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);

	// UI functions
	void UINewFrame(float deltaTime);
	void BuildUI();
	void CameraUI(std::shared_ptr<Camera> cam);
	void EntityUI(std::shared_ptr<GameEntity> entity);	
	void LightUI(Light& light);
	
	// Should the ImGui demo window be shown?
	bool showUIDemoWindow;

	// SSAO
	DirectX::XMFLOAT4 ssaoOffsets[64];
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetRTVs[RenderTargetType::RENDER_TARGET_TYPE_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> renderTargetSRVs[RenderTargetType::RENDER_TARGET_TYPE_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> randomTextureSRV;
	int ssaoSamples = 64;
	float ssaoRadius = 1.0f;

	// SSAO Shaders
	std::shared_ptr<SimplePixelShader> ssaoPS;
	std::shared_ptr<SimplePixelShader> ssaoBlurPS;
	std::shared_ptr<SimplePixelShader> ssaoCombinePS;
	std::shared_ptr<SimpleVertexShader> fullscreenVS;
};