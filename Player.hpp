#pragma once
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/EngineCommon.hpp"


//forward declarations
class Game;


class Player
{
//public member functions
public:
	//constructor and destructor
	Player(Game* owner);

	//game flow functions
	void Update();
	//void UpdateFromController();

	//player utilities
	Mat44 GetModelMatrix() const;

//public member variables
public:
	Camera m_playerCamera;

	float m_movementSpeed = 4.0f;
	float m_mouseTurnRate = 0.08f;
	//float m_controllerTurnRate = 120.0f;
	bool m_isSpeedUp;

	Game* m_game = nullptr;

	Vec3 m_position;
	Vec3 m_velocity;
	EulerAngles m_orientation;

	uint8_t m_blockIDToPlace = 1;
};
