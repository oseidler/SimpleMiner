#include "Game/Player.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"


//
//constructor
//
Player::Player(Game* owner)
	: m_game(owner)
{
}


//
//public game flow functions
//
void Player::Update()
{
	float systemDeltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	//block choosing
	if (g_theInput->WasKeyJustPressed('1'))
	{
		m_blockIDToPlace = 1;
	}
	if (g_theInput->WasKeyJustPressed('2'))
	{
		m_blockIDToPlace = 2;
	}
	if (g_theInput->WasKeyJustPressed('3'))
	{
		m_blockIDToPlace = 3;
	}
	if (g_theInput->WasKeyJustPressed('4'))
	{
		m_blockIDToPlace = 4;
	}
	if (g_theInput->WasKeyJustPressed('5'))
	{
		m_blockIDToPlace = 5;
	}
	if (g_theInput->WasKeyJustPressed('6'))
	{
		m_blockIDToPlace = 6;
	}
	if (g_theInput->WasKeyJustPressed('7'))
	{
		m_blockIDToPlace = 7;
	}
	if (g_theInput->WasKeyJustPressed('8'))
	{
		m_blockIDToPlace = 8;
	}
	if (g_theInput->WasKeyJustPressed('9'))
	{
		m_blockIDToPlace = 9;
	}

	m_isSpeedUp = false;

	Vec3 movementIntentions = Vec3();

	if (g_theInput->IsKeyDown('W'))
	{
		movementIntentions.x += 1.0f;
	}
	if (g_theInput->IsKeyDown('S'))
	{
		movementIntentions.x -= 1.0f;
	}
	if (g_theInput->IsKeyDown('A'))
	{
		movementIntentions.y += 1.0f;
	}
	if (g_theInput->IsKeyDown('D'))
	{
		movementIntentions.y -= 1.0f;
	}
	if (g_theInput->IsKeyDown('E'))
	{
		movementIntentions.z += 1.0f;
	}
	if (g_theInput->IsKeyDown('Q'))
	{
		movementIntentions.z -= 1.0f;
	}

	if (g_theInput->IsKeyDown(KEYCODE_SPACE))
	{
		m_isSpeedUp = true;
	}

	//get bases and clamp to XY
	Vec3 iBasis = GetModelMatrix().GetIBasis3D();
	iBasis.z = 0.0f;
	iBasis.Normalize();
	Vec3 jBasis = GetModelMatrix().GetJBasis3D();
	jBasis.z = 0.0f;
	jBasis.Normalize();

	m_velocity = (iBasis * movementIntentions.x + jBasis * movementIntentions.y + Vec3(0.0f, 0.0f, movementIntentions.z)) * m_movementSpeed;

	IntVec2 mouseDelta = g_theInput->GetCursorClientDelta();

	m_orientation.m_yawDegrees -= (float)mouseDelta.x * m_mouseTurnRate;
	m_orientation.m_pitchDegrees += (float)mouseDelta.y * m_mouseTurnRate;

	//UpdateFromController();

	if (m_isSpeedUp)
	{
		systemDeltaSeconds *= 10.0f;
	}

	m_position += m_velocity * systemDeltaSeconds;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);

	m_playerCamera.SetTransform(m_position, m_orientation);
}


//void Player::UpdateFromController()
//{
//	XboxController const& controller = g_theInput->GetController(0);
//	AnalogJoystick const& leftStick = controller.GetLeftStick();
//	AnalogJoystick const& rightStick = controller.GetRightStick();
//
//	float leftStickMagnitude = leftStick.GetMagnitude();
//	float rightStickMagnitude = rightStick.GetMagnitude();
//
//	Vec3 movementIntentions = Vec3();
//
//	if (leftStickMagnitude > 0.0f)
//	{
//		movementIntentions.y = -leftStick.GetPosition().x;
//		movementIntentions.x = leftStick.GetPosition().y;
//
//		Vec3 iBasis = GetModelMatrix().GetIBasis3D();
//		Vec3 jBasis = GetModelMatrix().GetJBasis3D();
//		m_velocity = (iBasis * movementIntentions.x + jBasis * movementIntentions.y) * m_movementSpeed;
//	}
//
//	if (controller.IsButtonDown(XBOX_BUTTON_L))
//	{
//		movementIntentions.z += 1.0f;
//		m_velocity += Vec3(0.0f, 0.0f, movementIntentions.z) * m_movementSpeed;
//	}
//	if (controller.IsButtonDown(XBOX_BUTTON_R))
//	{
//		movementIntentions.z -= 1.0f;
//		m_velocity += Vec3(0.0f, 0.0f, movementIntentions.z) * m_movementSpeed;
//	}
//
//	if (rightStickMagnitude > 0.0f)
//	{
//		m_angularVelocity.m_yawDegrees = -rightStick.GetPosition().x * m_controllerTurnRate;
//		m_angularVelocity.m_pitchDegrees = -rightStick.GetPosition().y * m_controllerTurnRate;
//	}
//
//	if (controller.GetLeftTrigger() > 0.0f)
//	{
//		m_angularVelocity.m_rollDegrees = - m_rollSpeed;
//	}
//	if (controller.GetRightTrigger() > 0.0f)
//	{
//		m_angularVelocity.m_rollDegrees = m_rollSpeed;
//	}
//
//	if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
//	{
//		m_position = Vec3();
//		m_orientation = EulerAngles();
//	}
//
//	if (controller.IsButtonDown(XBOX_BUTTON_A))
//	{
//		m_isSpeedUp = true;
//	}
//}


//
//public player utilities
//
Mat44 Player ::GetModelMatrix() const
{
	Mat44 modelMatrix = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();

	modelMatrix.SetTranslation3D(m_position);

	return modelMatrix;
}
