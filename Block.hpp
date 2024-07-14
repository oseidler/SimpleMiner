#pragma once
#include "Engine/Core/EngineCommon.hpp"


//bitflag definitions
constexpr uint8_t BLOCK_BIT_IS_SKY = 1;
constexpr uint8_t BLOCK_BIT_IS_LIGHT_DIRTY = 2;


class Block
{
//public member functions
public:
	//accessors
	uint8_t GetOutdoorLightLevel() const;
	uint8_t GetIndoorLightLevel() const;
	bool	IsSky() const;
	bool	IsLightDirty() const;
	bool	IsOpaque() const;
	bool	IsSolid() const;

	//mutators
	void SetOutdoorLightLevel(uint8_t outdoorLighting);
	void SetIndoorLightLevel(uint8_t indoorLighting);
	void SetIsSky(bool isSky);
	void SetIsLightDirty(bool isLightDirty);
	

//public member variables
public:
	uint8_t m_blockType = 0;
	uint8_t m_lighting = 0;
	uint8_t m_bitFlags = 0;
};
