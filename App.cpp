#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/JobSystem/JobSystem.hpp"


App* g_theApp = nullptr;

Renderer*	 g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
Window*		 g_theWindow = nullptr;

Game* m_theGame = nullptr;


//public game flow functions
void App::Startup()
{
	XmlDocument gameConfigXml;
	char const* filePath = "Data/GameConfig.xml";
	XmlError result = gameConfigXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open game config file!"));
	XmlElement* root = gameConfigXml.RootElement();
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*root);
	
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);
	
	InputSystemConfig inputSystemConfig;
	g_theInput = new InputSystem(inputSystemConfig);

	WindowConfig windowConfig;
	windowConfig.m_windowTitle = "SimpleMiner";
	windowConfig.m_clientAspect = g_gameConfigBlackboard.GetValue("windowAspect", 2.0f);
	windowConfig.m_inputSystem = g_theInput;
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(rendererConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_renderer = g_theRenderer;
	devConsoleConfig.m_camera = &m_devConsoleCamera;
	g_theDevConsole = new DevConsole(devConsoleConfig);
	
	AudioSystemConfig audioSystemConfig;
	g_theAudio = new AudioSystem(audioSystemConfig);

	JobSystemConfig jobSystemConfig;
	g_theJobSystem = new JobSystem(jobSystemConfig);
	
	g_theEventSystem->Startup();
	g_theDevConsole->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theAudio->Startup();
	g_theJobSystem->Startup();

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	DebugRenderSystemStartup(debugRenderConfig);

	m_theGame = new Game();
	m_theGame->Startup();

	SubscribeEventCallbackFunction("quit", Event_Quit);

	m_devConsoleCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_SIZE_X, SCREEN_CAMERA_SIZE_Y));

	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MAJOR, "Controls: ");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " W/S: Move Forward/Back");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " A/D: Move Left/Right");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Q/E: Move Up/Down");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Mouse: Look Around");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Space: Speed Up/Start Game");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " F8: Deactivate All Chunks");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Left Click: Dig Below");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Right Click: Place Bricks Below");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " Escape: Exit Game");
	g_theDevConsole->AddLine(DevConsole::COLOR_INFO_MINOR, " ~: Open/Close Dev Console");
}


void App::Run()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}


void App::Shutdown()
{
	m_theGame->Shutdown();
	delete m_theGame;
	m_theGame = nullptr;

	DebugRenderSystemShutdown();

	g_theJobSystem->Shutdown();
	delete g_theJobSystem;
	g_theJobSystem = nullptr;

	g_theAudio->Shutdown();
	delete g_theAudio;
	g_theAudio = nullptr;

	g_theRenderer->Shutdown();
	delete g_theRenderer;
	g_theRenderer = nullptr;

	g_theWindow->Shutdown();
	delete g_theWindow;
	g_theWindow = nullptr;

	g_theInput->Shutdown();
	delete g_theInput;
	g_theInput = nullptr;

	g_theDevConsole->Shutdown();
	delete g_theDevConsole;
	g_theDevConsole = nullptr;

	g_theEventSystem->Shutdown();
	delete g_theEventSystem;
	g_theEventSystem = nullptr;
}


void App::RunFrame()
{
	//tick the system clock
	Clock::TickSystemClock();

	//run through the four parts of the frame
	BeginFrame();
	Update();
	Render();
	EndFrame();
}


//
//public app utilities
//
bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return m_isQuitting;
}


//
//static app utilities
//
bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);

	if (g_theApp != nullptr)
	{
		g_theApp->HandleQuitRequested();
	}

	return true;
}


//
//private game flow functions
//
void App::BeginFrame()
{
	g_theEventSystem->BeginFrame();
	g_theDevConsole->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
	g_theJobSystem->BeginFrame();

	DebugRenderBeginFrame();
}


void App::Update()
{
	//quit or leave attract mode if q is pressed
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		if (m_theGame->m_isAttractMode)
		{
			HandleQuitRequested();
		}
		else
		{
			RestartGame();
		}
	}

	//set mouse state based on game
	if (!g_theDevConsole->IsOpen() && Window::GetWindowContext()->HasFocus() && !m_theGame->m_isAttractMode)
	{
		g_theInput->SetCursorMode(true, true);
	}
	else
	{
		g_theInput->SetCursorMode(false, false);
	}

	//update the game
	m_theGame->Update();

	//go back to the start if the game finishes
	if (m_theGame->m_isFinished)
	{
		RestartGame();
	}
}


void App::Render() const
{	
	m_theGame->Render();

	//render dev console separately from and after rest of game
	g_theRenderer->BeginCamera(m_devConsoleCamera);
	g_theDevConsole->Render(AABB2(0.0f, 0.0f, SCREEN_CAMERA_SIZE_X * 0.9f, SCREEN_CAMERA_SIZE_Y * 0.9f));
	g_theRenderer->EndCamera(m_devConsoleCamera);
}


void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theDevConsole->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theAudio->EndFrame();
	g_theJobSystem->EndFrame();

	DebugRenderEndFrame();
}


//
//private app utilities
//
void App::RestartGame()
{
	//delete old game
	m_theGame->Shutdown();
	delete m_theGame;
	m_theGame = nullptr;

	g_theJobSystem->Shutdown();
	delete g_theJobSystem;
	g_theJobSystem = nullptr;

	//initialize new game
	JobSystemConfig jobSystemConfig;
	g_theJobSystem = new JobSystem(jobSystemConfig);
	g_theJobSystem->Startup();

	m_theGame = new Game();
	m_theGame->Startup();
}
