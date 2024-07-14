#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/BlockTemplate.hpp"
#include "Game/App.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/JobSystem/JobSystem.hpp"


//game flow functions
void Game::Startup()
{
	//load game assets
	LoadAssets();

	//initialize world shader
	g_worldShader = g_theRenderer->CreateShader("Data/Shaders/World");
	m_gameCBO = g_theRenderer->CreateConstantBuffer(sizeof(ShaderGameConstants));
	
	//create world
	BlockDefinition::InitializeBlockDefs();
	m_world = new World(this);

	//initialize block templates
	BlockTemplate::InitializeAllTemplates();

	//set camera bounds
	m_world->m_player->m_playerCamera.SetOrthoView(Vec2(WORLD_CAMERA_MIN_X, WORLD_CAMERA_MIN_Y), Vec2(WORLD_CAMERA_MAX_X, WORLD_CAMERA_MAX_Y));
	m_world->m_player->m_playerCamera.SetPerspectiveView(2.0f, 60.0f, 0.1f, 1024.0f);
	m_world->m_player->m_playerCamera.SetRenderBasis(Vec3(0.0f, 0.0f, 1.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_SIZE_X, SCREEN_CAMERA_SIZE_Y));

	//add world axes
	DebugAddWorldArrow(Vec3(), Vec3(1.0f, 0.0f, 0.0f), 0.01f, -1.0f, Rgba8(255, 0, 0), Rgba8(255, 0, 0));
	DebugAddWorldArrow(Vec3(), Vec3(0.0f, 1.0f, 0.0f), 0.01f, -1.0f, Rgba8(0, 255, 0), Rgba8(0, 255, 0));
	DebugAddWorldArrow(Vec3(), Vec3(0.0f, 0.0f, 1.0f), 0.01f, -1.0f, Rgba8(0, 0, 255), Rgba8(0, 0, 255));

	//set hidden surface removal mode
	g_enableHiddenSurfaceRemoval = g_gameConfigBlackboard.GetValue("enableHiddenSurfaceRemoval", true);

	//start chunk generation threads
	//create new worker threads using job system
	int numWorkerThreads = std::thread::hardware_concurrency() - 1;

	g_theJobSystem->CreateWorkers(numWorkerThreads);

	EnterAttractMode();
}


void Game::Update()
{
	//if in attract mode, just update that and don't bother with anything else
	if (m_isAttractMode)
	{
		UpdateAttract();

		return;
	}
	
	//turn debug view on when f1 is pressed
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isDebugView = !m_isDebugView;
	}

	//check if p is pressed to toggle pause
	/*if (g_theInput->WasKeyJustPressed('P'))
	{
		m_gameClock.TogglePause();
	}*/

	//check if t is currently being held to turn slow-mo on
	/*if (g_theInput->IsKeyDown('T'))
	{
		m_gameClock.SetTimeScale(0.1f);
	}
	else
	{
		m_gameClock.SetTimeScale(1.0f);
	}*/

	////check if o is pressed and do single step if so
	//if (g_theInput->WasKeyJustPressed('O'))
	//{
	//	m_gameClock.StepSingleFrame();
	//}

	Clock& sysClock = Clock::GetSystemClock();
	std::string gameInfo = Stringf("Time: %.2f  MSPF: %.1f  FPS: %.1f", sysClock.GetTotalSeconds(), sysClock.GetDeltaSeconds() * 1000.0f, 1.0f / sysClock.GetDeltaSeconds());
	DebugAddScreenText(gameInfo, Vec2(SCREEN_CAMERA_SIZE_X, SCREEN_CAMERA_SIZE_Y), 16.0f, Vec2(1.0f, 1.0f), 0.0f, Rgba8(), Rgba8());

	Vec3& pos = m_world->m_player->m_position;
	std::string posMessage = Stringf("Player position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
	DebugAddMessage(posMessage, 0.0f);

	//update world
	m_world->Update();

	std::string chunkInfo = Stringf("Chunks: %i / %i", m_world->m_activeChunks.size(), 1024);
	DebugAddMessage(chunkInfo, 0.0f);
}


void Game::Render() const
{
	//if in attract mode, just render that and not anything else
	if (m_isAttractMode)
	{
		RenderAttract();

		return;
	}
	
	g_theRenderer->BeginCamera(m_world->m_player->m_playerCamera);	//render game world with the world camera

	//render world
	m_world->Render();

	g_theRenderer->EndCamera(m_world->m_player->m_playerCamera);

	//debug world rendering
	DebugRenderWorld(m_world->m_player->m_playerCamera);

	//g_theRenderer->BeginCamera(m_screenCamera);	//render UI with the screen camera

	//screen camera rendering here

	//g_theRenderer->EndCamera(m_screenCamera);

	//debug screen rendering
	DebugRenderScreen(m_screenCamera);
}


void Game::Shutdown()
{
	//deactivate all chunks
	m_world->DeactivateAllChunks();
	
	//delete all allocated pointers here
	if (m_world != nullptr)
	{
		delete m_world;
	}

	if (m_gameCBO != nullptr)
	{
		delete m_gameCBO;
	}

	//clear the job system of all its leftover jobs
	g_theJobSystem->ClearJobSystem();
}


void Game::SetShaderGameConstants(Vec3 camPosition, Rgba8 indoorLightColor, Rgba8 outdoorLightColor, Rgba8 skyColor, float fogFarDistance, float fogNearDistance)
{
	ShaderGameConstants gameConstants;
	gameConstants.CameraWorldPosition = Vec4(camPosition.x, camPosition.y, camPosition.z, 1.0f);
	gameConstants.IndoorLightColor = Vec4(NormalizeByte(indoorLightColor.r), NormalizeByte(indoorLightColor.g), NormalizeByte(indoorLightColor.b), 1.0f);
	gameConstants.OutdoorLightColor = Vec4(NormalizeByte(outdoorLightColor.r), NormalizeByte(outdoorLightColor.g), NormalizeByte(outdoorLightColor.b), 1.0f);
	gameConstants.SkyColor = Vec4(NormalizeByte(skyColor.r), NormalizeByte(skyColor.g), NormalizeByte(skyColor.b), 1.0f);
	gameConstants.FogFarDistance = fogFarDistance;
	gameConstants.FogNearDistance = fogNearDistance;

	g_theRenderer->CopyCPUToGPU(&gameConstants, sizeof(gameConstants), m_gameCBO);
	g_theRenderer->BindConstantBuffer(4, m_gameCBO);
}


void Game::SetShaderGameConstants(Vec3 camPosition, Rgba8 indoorLightColor, Rgba8 outdoorLightColor, Rgba8 skyColor)
{
	ShaderGameConstants gameConstants;
	gameConstants.CameraWorldPosition = Vec4(camPosition.x, camPosition.y, camPosition.z, 1.0f);
	gameConstants.IndoorLightColor = Vec4(NormalizeByte(indoorLightColor.r), NormalizeByte(indoorLightColor.g), NormalizeByte(indoorLightColor.b), 1.0f);
	gameConstants.OutdoorLightColor = Vec4(NormalizeByte(outdoorLightColor.r), NormalizeByte(outdoorLightColor.g), NormalizeByte(outdoorLightColor.b), 1.0f);
	gameConstants.SkyColor = Vec4(NormalizeByte(skyColor.r), NormalizeByte(skyColor.g), NormalizeByte(skyColor.b), 1.0f);

	g_theRenderer->CopyCPUToGPU(&gameConstants, sizeof(gameConstants), m_gameCBO);
	g_theRenderer->BindConstantBuffer(4, m_gameCBO);
}


//
//game flow sub-functions
//
void Game::UpdateAttract()
{
	XboxController const& controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->WasKeyJustPressed('N') || controller.WasButtonJustPressed(XBOX_BUTTON_START) || controller.WasButtonJustPressed(XBOX_BUTTON_A))
	{
		//g_theAudio->StartSound(m_testSound);
		EnterGameplay();
	}
}


void Game::RenderAttract() const
{
	g_theRenderer->ClearScreen(Rgba8(255, 0, 255));	//clear screen to magenta
	
	g_theRenderer->BeginCamera(m_screenCamera);	//render attract screen with the screen camera
	
	//example attract screen rendering
	float currentTime = m_gameClock.GetTotalSeconds();
	float ringRadius = 250.f + cosf(currentTime);
	float ringWidth = 10.f * sinf(currentTime);
	Rgba8 ringColor = Rgba8(255, 135, 50);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetModelConstants();
	DebugDrawRing(Vec2(SCREEN_CAMERA_CENTER_X, SCREEN_CAMERA_CENTER_Y), ringRadius, ringWidth, ringColor);

	g_theRenderer->EndCamera(m_screenCamera);
}


//
//mode switching functions
//
void Game::EnterAttractMode()
{
	m_isAttractMode = true;
}


void Game::EnterGameplay()
{
	m_isAttractMode = false;
}


//
//asset management functions
//
void Game::LoadAssets()
{
	LoadSounds();
	LoadTextures();
}


void Game::LoadSounds()
{
	//m_testSound = g_theAudio->CreateOrGetSound("Data/Audio/TestSound.mp3");
}


void Game::LoadTextures()
{
	g_worldSpriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/BasicSprites_64x64.png");
	g_worldSpriteSheet = new SpriteSheet(*g_worldSpriteSheetTexture, IntVec2(WORLD_SPRITE_SHEET_WIDTH, WORLD_SPRITE_SHEET_HEIGHT));
}
