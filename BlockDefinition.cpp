#include "Game/BlockDefinition.hpp"
#include "Game/GameCommon.hpp"


//static variable declaration
std::vector<BlockDefinition> BlockDefinition::s_blockDefs;


//
//constructor
//
BlockDefinition::BlockDefinition(std::string name, bool isVisible, bool isSolid, bool isOpaque, IntVec2 topSpriteCoords, IntVec2 sideSpriteCoords, IntVec2 bottomSpriteCoords, int lightEmissionValue)
	: m_name(name)
	, m_isVisible(isVisible)
	, m_isSolid(isSolid)
	, m_isOpaque(isOpaque)
	, m_topSpriteCoords(topSpriteCoords)
	, m_sideSpriteCoords(sideSpriteCoords)
	, m_bottomSpriteCoords(bottomSpriteCoords)
	, m_lightEmissionValue(lightEmissionValue)
{
	m_topSpriteIndex = m_topSpriteCoords.x + m_topSpriteCoords.y * WORLD_SPRITE_SHEET_WIDTH;
	m_sideSpriteIndex = m_sideSpriteCoords.x + m_sideSpriteCoords.y * WORLD_SPRITE_SHEET_WIDTH;
	m_bottomSpriteIndex = m_bottomSpriteCoords.x + m_bottomSpriteCoords.y * WORLD_SPRITE_SHEET_WIDTH;
}


//
//static functions
//
void BlockDefinition::InitializeBlockDefs()
{
	if (s_blockDefs.size() > 0)
	{
		return;
	}

	s_blockDefs.push_back(BlockDefinition("air", false, false, false, IntVec2(), IntVec2(), IntVec2()));
	s_blockDefs.push_back(BlockDefinition("stone", true, true, true, IntVec2(33, 32), IntVec2(33, 32), IntVec2(33, 32)));
	s_blockDefs.push_back(BlockDefinition("dirt", true, true, true, IntVec2(32, 34), IntVec2(32, 34), IntVec2(32, 34)));
	s_blockDefs.push_back(BlockDefinition("grass", true, true, true, IntVec2(32, 33), IntVec2(33, 33), IntVec2(32, 34)));
	s_blockDefs.push_back(BlockDefinition("cobblestone", true, true, true, IntVec2(35, 32), IntVec2(35, 32), IntVec2(35, 32)));
	s_blockDefs.push_back(BlockDefinition("glowstone", true, true, true, IntVec2(46, 34), IntVec2(46, 34), IntVec2(46, 34), 15));
	s_blockDefs.push_back(BlockDefinition("sand", true, true, true, IntVec2(34, 34), IntVec2(34, 34), IntVec2(34, 34)));
	s_blockDefs.push_back(BlockDefinition("logoak", true, true, true, IntVec2(38, 33), IntVec2(36, 33), IntVec2(38, 33)));
	s_blockDefs.push_back(BlockDefinition("logspruce", true, true, true, IntVec2(38, 33), IntVec2(37, 33), IntVec2(38, 33)));
	s_blockDefs.push_back(BlockDefinition("cactus", true, true, true, IntVec2(39, 36), IntVec2(37, 36), IntVec2(39, 36)));
	s_blockDefs.push_back(BlockDefinition("coal", true, true, true, IntVec2(63, 34), IntVec2(63, 34), IntVec2(63, 34)));
	s_blockDefs.push_back(BlockDefinition("iron", true, true, true, IntVec2(63, 35), IntVec2(63, 35), IntVec2(63, 35)));
	s_blockDefs.push_back(BlockDefinition("gold", true, true, true, IntVec2(63, 36), IntVec2(63, 36), IntVec2(63, 36)));
	s_blockDefs.push_back(BlockDefinition("diamond", true, true, true, IntVec2(63, 37), IntVec2(63, 37), IntVec2(63, 37)));
	s_blockDefs.push_back(BlockDefinition("water", true, true, true, IntVec2(32, 44), IntVec2(32, 44), IntVec2(32, 44)));
	s_blockDefs.push_back(BlockDefinition("ice", true, true, true, IntVec2(45, 34), IntVec2(45, 34), IntVec2(45, 34)));
	s_blockDefs.push_back(BlockDefinition("leafoak", true, true, true, IntVec2(32, 35), IntVec2(32, 35), IntVec2(32, 35)));
	s_blockDefs.push_back(BlockDefinition("leafspruce", true, true, true, IntVec2(34, 35), IntVec2(34, 35), IntVec2(34, 35)));
	s_blockDefs.push_back(BlockDefinition("lava", true, true, true, IntVec2(48, 43), IntVec2(48, 43), IntVec2(48, 43), 7));
	s_blockDefs.push_back(BlockDefinition("volcanicrock", true, true, true, IntVec2(48, 41), IntVec2(48, 41), IntVec2(48, 41), 2));
	s_blockDefs.push_back(BlockDefinition("mushroomstem", true, true, true, IntVec2(38, 34), IntVec2(37, 34), IntVec2(38, 34)));
	s_blockDefs.push_back(BlockDefinition("mushroomblock", true, true, true, IntVec2(39, 34), IntVec2(39, 34), IntVec2(39, 34)));
}


BlockDefinition const* BlockDefinition::GetBlockDefFromID(int blockDefID)
{
	if (blockDefID >= s_blockDefs.size())
	{
		return nullptr;
	}

	return &s_blockDefs[blockDefID];
}


BlockDefinition const* BlockDefinition::GetBlockDefFromName(std::string name)
{
	for (int defIndex = 0; defIndex < s_blockDefs.size(); defIndex++)
	{
		if (s_blockDefs[defIndex].m_name == name)
		{
			return &s_blockDefs[defIndex];
		}
	}

	//return null if it wasn't found
	return nullptr;
}


int BlockDefinition::GetBlockDefIDFromName(std::string name)
{
	for (int defIndex = 0; defIndex < s_blockDefs.size(); defIndex++)
	{
		if (s_blockDefs[defIndex].m_name == name)
		{
			return defIndex;
		}
	}

	//return air if it wasn't found
	return 0;
}


std::string BlockDefinition::GetBlockDefNameFromID(int blockDefID)
{
	if (blockDefID >= s_blockDefs.size())
	{
		return "invalid";
	}

	return s_blockDefs[blockDefID].m_name;
}
