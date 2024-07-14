#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"


class BlockDefinition
{
//public member functions
public:
	//constructor
	BlockDefinition(std::string name, bool isVisible, bool isSolid, bool isOpaque, IntVec2 topSpriteCoords, IntVec2 sideSpriteCoords, IntVec2 bottomSpriteCoords, int lightEmissionValue = 0);
	
	//static functions
	static void InitializeBlockDefs();
	static BlockDefinition const* GetBlockDefFromID(int blockDefID);
	static BlockDefinition const* GetBlockDefFromName(std::string name);
	static int GetBlockDefIDFromName(std::string name);
	static std::string GetBlockDefNameFromID(int blockDefID);

//public member variables
public:
	static std::vector<BlockDefinition> s_blockDefs;

	//block parameters
	std::string m_name = "invalid name";
	bool m_isVisible = false;
	bool m_isSolid = false;
	bool m_isOpaque = false;
	IntVec2 m_topSpriteCoords = IntVec2();
	IntVec2 m_sideSpriteCoords = IntVec2();
	IntVec2 m_bottomSpriteCoords = IntVec2();
	int m_topSpriteIndex = -1;
	int m_sideSpriteIndex = -1;
	int m_bottomSpriteIndex = -1;
	int  m_lightEmissionValue = 0;
};
