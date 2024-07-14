#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Camera.hpp"

class App
{
//public member functions
public:
	//game flow functions
	void Startup();
	void Run();
	void Shutdown();
	void RunFrame();

	//app utilities
	bool IsQuitting() const { return m_isQuitting; }
	bool HandleQuitRequested();

	//static app utilites
	static bool Event_Quit(EventArgs& args);

//private member variables
private:
	//game flow functions
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	//app utilities
	void RestartGame();

//private member variables
private:
	bool m_isQuitting = false;
	Camera m_devConsoleCamera;
};
