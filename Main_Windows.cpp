#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places


//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE , HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED( commandLineString );

	g_theApp = new App();
	g_theApp->Startup();

	g_theApp->Run();

	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}
