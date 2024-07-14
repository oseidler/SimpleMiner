#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/Vec4.hpp"


//forward declarations
class World;
class ConstantBuffer;


//world shader constant buffer struct
struct ShaderGameConstants
{
	Vec4  CameraWorldPosition;
	Vec4  IndoorLightColor = Vec4(1.0f, 0.9f, 0.8f, 1.0f);
	Vec4  OutdoorLightColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	Vec4  SkyColor = Vec4();
	float FogFarDistance = g_gameConfigBlackboard.GetValue("chunkActivationDistance", 250.0f) - 16.0f;
	float FogNearDistance = FogFarDistance * 0.5f;
	Vec2  SizeFiller;
};


class Game 
{
//public member functions
public:
	//game flow functions
	void Startup();
	void Update();
	void Render() const;
	void Shutdown();

	//rendering functions
	void SetShaderGameConstants(Vec3 camPosition, Rgba8 indoorLightColor, Rgba8 outdoorLightColor, Rgba8 skyColor, float fogFarDistance, float fogNearDistance);
	void SetShaderGameConstants(Vec3 camPosition, Rgba8 indoorLightColor, Rgba8 outdoorLightColor, Rgba8 skyColor);

//public member variables
public:
	//game state bools
	bool		m_isFinished = false;
	bool		m_isAttractMode = true;

	//sounds
	//SoundID m_testSound;

	//game objects
	Clock	m_gameClock = Clock();
	World*	m_world = nullptr;

	//world constant buffer
	ConstantBuffer* m_gameCBO = nullptr;

//private member functions
private:
	//game flow sub-functions
	void UpdateAttract();
	void RenderAttract() const;

	//mode-switching functions
	void EnterAttractMode();
	void EnterGameplay();

	//asset management functions
	void LoadAssets();
	void LoadSounds();
	void LoadTextures();

//private member variables
private:
	//game state bools
	bool	m_isDebugView = false;

	//camera variables
	//Camera m_worldCamera;
	Camera m_screenCamera;
};
