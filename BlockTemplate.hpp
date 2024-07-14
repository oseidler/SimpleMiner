#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec3.hpp"


struct BlockTemplateEntry
{
	BlockTemplateEntry(std::string name, IntVec3 coords) : m_blockName(name), m_localBlockCoords(coords) {}
	
	std::string m_blockName = "air";
	IntVec3 m_localBlockCoords = IntVec3();
};


class BlockTemplate
{
//public member functions
public:
	//initialization function for each BlockTemplate type
	static void InitializeOakTreeTemplate();
	static void InitializeSpruceTreeTemplate();
	static void InitializeCactusTemplate();
	static void InitializeLavaPitTemplate();
	static void InitializeGiantMushroom();
	//static void InitializeTemplateFromFile(std::string const& templateFilePath);
	static void InitializeAllTemplates();

	//accessor
	static BlockTemplate const* GetBlockTemplateByName(std::string const& name);

//public member variables
public:
	std::string m_name = "";
	std::vector<BlockTemplateEntry> m_blueprint;

	static std::vector<BlockTemplate> s_loadedTemplates;
};
