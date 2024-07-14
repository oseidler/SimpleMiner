#include "Game/BlockTemplate.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Math/MathUtils.hpp"


std::vector<BlockTemplate> BlockTemplate::s_loadedTemplates;


//
//static initialization functions
//
void BlockTemplate::InitializeOakTreeTemplate()
{
	BlockTemplate blockTemplate;
	blockTemplate.m_name = "Oak Tree";
	
	for (int blockZ = 0; blockZ < 6; blockZ++)
	{
		blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("logoak", IntVec3(0, 0, blockZ)));
	}
	
	for (int blockX = -2; blockX < 3; blockX++)
	{
		for (int blockY = -2; blockY < 3; blockY++)
		{
			if (blockX != 0 || blockY != 0)
			{
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafoak", IntVec3(blockX, blockY, 3)));
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafoak", IntVec3(blockX, blockY, 4)));
			}
		}
	}
	
	for (int blockX = -1; blockX < 2; blockX++)
	{
		for (int blockY = -1; blockY < 2; blockY++)
		{
			if (blockX != 0 || blockY != 0)
			{
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafoak", IntVec3(blockX, blockY, 5)));
			}
		}
	}

	blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafoak", IntVec3(0, 0, 6)));

	s_loadedTemplates.emplace_back(blockTemplate);
}


void BlockTemplate::InitializeSpruceTreeTemplate()
{
	BlockTemplate blockTemplate;
	blockTemplate.m_name = "Spruce Tree";

	for (int blockZ = 0; blockZ < 8; blockZ++)
	{
		blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("logspruce", IntVec3(0, 0, blockZ)));
	}

	for (int blockX = -2; blockX < 3; blockX++)
	{
		for (int blockY = -2; blockY < 3; blockY++)
		{
			if (blockX != 0 || blockY != 0)
			{
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 3)));
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 4)));
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 5)));
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 6)));
			}
		}
	}

	for (int blockX = -1; blockX < 2; blockX++)
	{
		for (int blockY = -1; blockY < 2; blockY++)
		{
			if (blockX != 0 || blockY != 0)
			{
				blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 7)));
			}

			blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(blockX, blockY, 8)));
		}
	}

	blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("leafspruce", IntVec3(0, 0, 9)));

	s_loadedTemplates.emplace_back(blockTemplate);
}


void BlockTemplate::InitializeCactusTemplate()
{
	BlockTemplate blockTemplate;
	blockTemplate.m_name = "Cactus";
	
	for (int blockZ = 0; blockZ < 4; blockZ++)
	{
		blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("cactus", IntVec3(0, 0, blockZ)));
	}

	s_loadedTemplates.emplace_back(blockTemplate);
}


void BlockTemplate::InitializeLavaPitTemplate()
{
	BlockTemplate blockTemplate;
	blockTemplate.m_name = "Lava Pit";

	for (int blockZ = -7; blockZ < 8; blockZ++)
	{
		for (int blockY = -7; blockY < 8; blockY++)
		{
			for (int blockX = -7; blockX < 8; blockX++)
			{
				float blockXCenter = static_cast<float>(blockX) + 0.5f;
				float blockYCenter = static_cast<float>(blockY) + 0.5f;
				float blockZCenter = static_cast<float>(blockZ) + 0.5f;

				if (IsPointInsideSphere3D(Vec3(blockXCenter, blockYCenter, blockZCenter), Vec3(), 5.0f))
				{
					if (blockZ >= 0)
					{
						blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("air", IntVec3(blockX, blockY, blockZ)));
					}
					else
					{
						blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("lava", IntVec3(blockX, blockY, blockZ)));
					}
				}
				else if (IsPointInsideSphere3D(Vec3(blockXCenter, blockYCenter, blockZCenter), Vec3(), 7.0f))
				{
					if (blockZ >= 0)
					{
						blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("air", IntVec3(blockX, blockY, blockZ)));
					}
					else
					{
						blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("volcanicrock", IntVec3(blockX, blockY, blockZ)));
					}
				}
			}
		}
	}

	s_loadedTemplates.emplace_back(blockTemplate);
}


void BlockTemplate::InitializeGiantMushroom()
{
	BlockTemplate blockTemplate;
	blockTemplate.m_name = "Giant Mushroom";

	for (int blockZ = 0; blockZ < 9; blockZ++)
	{
		for (int blockY = -4; blockY < 5; blockY++)
		{
			for (int blockX = -4; blockX < 5; blockX++)
			{
				if (blockX < 2 && blockX > -2 && blockY < 2 && blockY > -2 && blockZ < 7)
				{
					blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("mushroomstem", IntVec3(blockX, blockY, blockZ)));
				}
				else if (blockZ > 2 && blockZ < 8 && !(abs(blockX) == 4 && abs(blockY) == 4))
				{
					blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("mushroomblock", IntVec3(blockX, blockY, blockZ)));
				}
				else if (blockZ == 8 && blockX < 4 && blockX > -4 && blockY < 4 && blockY > -4)
				{
					blockTemplate.m_blueprint.emplace_back(BlockTemplateEntry("mushroomblock", IntVec3(blockX, blockY, blockZ)));
				}
			}
		}
	}

	s_loadedTemplates.emplace_back(blockTemplate);
}


//void BlockTemplate::InitializeTemplateFromFile(std::string const& templateFilePath)
//{
//	//
//	//INCOMPLETE
//	//
//	
//	BlockTemplate blockTemplate;
//
//	//read file
//	std::vector<uint8_t> fileBuffer;
//	FileReadToBuffer(fileBuffer, templateFilePath);
//
//	//get name from file
//
//
//	//get each block from file
//
//
//	s_loadedTemplates.emplace_back(blockTemplate);
//}


void BlockTemplate::InitializeAllTemplates()
{
	if (s_loadedTemplates.size() > 0)
	{
		return;
	}

	InitializeCactusTemplate();
	InitializeOakTreeTemplate();
	InitializeSpruceTreeTemplate();

	InitializeLavaPitTemplate();
	InitializeGiantMushroom();
}


//
//static accessors
//
BlockTemplate const* BlockTemplate::GetBlockTemplateByName(std::string const& name)
{
	for (int templateIndex = 0; templateIndex < s_loadedTemplates.size(); templateIndex++)
	{
		if (s_loadedTemplates[templateIndex].m_name == name)
		{
			return &s_loadedTemplates[templateIndex];
		}
	}

	//return null if it wasn't found
	return nullptr;
}
