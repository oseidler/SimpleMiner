#include "Game/Chunk.hpp"
#include "Game/World.hpp"
#include "Game/GameCommon.hpp"
#include "Game/BlockDefinition.hpp"
#include "ThirdParty/Squirrel/RawNoise.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"


//
//constructor and destructor
//
Chunk::Chunk(IntVec2 chunkCoords, World* world)
	: m_chunkCoords(chunkCoords)
	, m_world(world)
{
	m_blocks = new Block[CHUNK_TOTAL_BLOCKS];
	
	float xMin = static_cast<float>(m_chunkCoords.x * CHUNK_SIZE_X);
	float yMin = static_cast<float>(m_chunkCoords.y * CHUNK_SIZE_Y);
	float zMin = 0.0f;
	float xMax = xMin + static_cast<float>(CHUNK_SIZE_X);
	float yMax = yMin + static_cast<float>(CHUNK_SIZE_Y);
	float zMax = zMin + static_cast<float>(CHUNK_SIZE_Z);
	m_bounds = AABB3(xMin, yMin, zMin, xMax, yMax, zMax);

	m_gpuMesh = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCU));
	m_cpuMesh.reserve(5000);
}


Chunk::~Chunk()
{
	if (m_gpuMesh != nullptr)
	{
		delete m_gpuMesh;
		m_gpuMesh = nullptr;
	}

	delete[] m_blocks;
}


//
//public game flow functions
//
void Chunk::Update()
{
	bool allNeighborsPresent = (m_eastNeighbor != nullptr) && (m_westNeighbor != nullptr) && (m_northNeighbor != nullptr) && (m_southNeighbor != nullptr);

	if (m_areVertsDirty && allNeighborsPresent)
	{
		RebuildVertexes();
	}
}


void Chunk::Render() const
{
	//setting renderer mode to opaque for now because transparent blocks (e.g. ice) look weird
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->DrawVertexBuffer(m_gpuMesh, static_cast<int>(m_cpuMesh.size()));
}


//
//public chunk utilities
//
void Chunk::PopulateBlocks()
{
	std::vector<BlockTemplateEntry> blockTemplateStartingPositions;

	unsigned int currentSeed = 0;

	//go outside bounds of chunk for tree generation
	for (int localY = -5; localY < CHUNK_SIZE_Y + 5; localY++)
	{
		for (int localX = -5; localX < CHUNK_SIZE_X + 5; localX++)
		{
			int globalX = localX + (m_chunkCoords.x * CHUNK_SIZE_X);
			int globalY = localY + (m_chunkCoords.y * CHUNK_SIZE_Y);

			float globalXFloat = static_cast<float>(globalX);
			float globalYFloat = static_cast<float>(globalY);

			currentSeed = m_world->m_worldSeed + 1;

			//determine biome factors using noise
			float humidity = 0.5f + 0.5f * Compute2dPerlinNoise(globalXFloat, globalYFloat, 400.0f, 5, 0.5f, 2.0f, true, currentSeed++);
			float temperature = 0.5f + 0.5f * Compute2dPerlinNoise(globalXFloat, globalYFloat, 400.0f, 5, 0.5f, 2.0f, true, currentSeed++);
			temperature += 0.007f * Get2dNoiseNegOneToOne(globalX, globalY, currentSeed);
			float hilliness = MAX_HILLINESS * SmoothStep3((0.5f + 0.5f * Compute2dPerlinNoise(globalXFloat, globalYFloat, 400.0f, 2, 0.5f, 2.0f, true, currentSeed++)));
			float oceanness = SmoothStep3(Compute2dPerlinNoise(globalXFloat, globalYFloat, 1200.0f, 3, 0.5f, 4.0f, true, currentSeed++));

			float treeDensity = 0.5f + 0.5f * Compute2dPerlinNoise(globalXFloat, globalYFloat, 500.0f, 4, 0.5f, 2.0f, true, currentSeed++);
			//generate tree noises in 5x5 grid (index 13 is the center one)
			float treeNoise[25] = { 0.0f };
			for (int treeY = -2; treeY < 3; treeY++)
			{
				for (int treeX = -2; treeX < 3; treeX++)
				{
					int treeArrayIndex = (treeX + 2) + ((treeY + 2) * 5);
					treeNoise[treeArrayIndex] = 0.5f + 0.5f * Compute2dPerlinNoise(static_cast<float>(globalX + treeX), static_cast<float>(globalY + treeY), 400.0f, 8, treeDensity, 2.0f, true, currentSeed);
				}
			}
			currentSeed++;
			bool isHighestTreeNoiseInGrid = true;
			for (int gridIndex = 0; gridIndex < 25; gridIndex++)
			{
				if (gridIndex != 13 && treeNoise[gridIndex] > treeNoise[13])
				{
					isHighestTreeNoiseInGrid = false;
				}
			}

			//generate mushroom noises in 15x15 grid (index 113 is the center one)
			float mushroomNoise[225] = { 0.0f };
			for (int mushroomY = -7; mushroomY < 8; mushroomY++)
			{
				for (int mushroomX = -7; mushroomX < 8; mushroomX++)
				{
					int mushroomArrayIndex = (mushroomX + 7) + ((mushroomY + 7) * 15);
					mushroomNoise[mushroomArrayIndex] = 0.5f + 0.5f * Compute2dPerlinNoise(static_cast<float>(globalX + mushroomX), static_cast<float>(globalY + mushroomY), 300.0f, 8, 0.5f, 2.0f, true, currentSeed);
				}
			}
			currentSeed++;
			bool isHighestMushroomNoiseInGrid = true;
			if (mushroomNoise[113] < MUSHROOM_BASE_THRESHOLD)
			{
				isHighestMushroomNoiseInGrid = false;
			}
			else
			{
				for (int gridIndex = 0; gridIndex < 225; gridIndex++)
				{
					if (gridIndex != 113 && mushroomNoise[gridIndex] > mushroomNoise[113])
					{
						isHighestMushroomNoiseInGrid = false;
					}
				}
			}

			int sandThickness = static_cast<int>(RangeMapClamped(humidity, 0.0f, HUMIDITY_SAND_THRESHOLD, MAX_SAND_THICKNESS, 0.0f));
			int iceThickness = static_cast<int>(RangeMapClamped(temperature, 0.0f, TEMPERATURE_ICE_THRESHOLD, MAX_ICE_THICKNESS, 0.0f));

			//use noise to determine terrain height
			int terrainHeightZ = BASE_TERRAIN_HEIGHT + static_cast<int>(hilliness * fabsf(Compute2dPerlinNoise(globalXFloat, globalYFloat, 200.0f, 5, 0.5f, 2.0f, true, m_world->m_worldSeed)));

			//lower terrain height based on oceanness
			if (oceanness > MAX_OCEANNESS_THRESHOLD)
			{
				terrainHeightZ -= OCEAN_FLOOR_DEPTH;
			}
			else if (oceanness <= MAX_OCEANNESS_THRESHOLD && oceanness > 0.0f)
			{
				float lerpedOceanDepthFraction = SmoothStart5(RangeMap(oceanness, 0.0f, MAX_OCEANNESS_THRESHOLD, 0.0f, 1.0f));
				terrainHeightZ = static_cast<int>(Interpolate(static_cast<float>(terrainHeightZ), static_cast<float>(terrainHeightZ - OCEAN_FLOOR_DEPTH), lerpedOceanDepthFraction));
			}

			float dirtDepthNoise = Get2dNoiseZeroToOne(globalX, globalY, currentSeed++);
			int dirtDepth = 3;
			if (dirtDepthNoise > 0.5f)
			{
				dirtDepth = 4;
			}
			int stoneHeightZ = terrainHeightZ - dirtDepth;

			for (int localZ = 0; localZ < CHUNK_SIZE_Z; localZ++)
			{
				if (localX >= 0 && localX < CHUNK_SIZE_X && localY >= 0 && localY < CHUNK_SIZE_Y)
				{
					//top block of terrain is grass
					if (localZ == terrainHeightZ)
					{
						if (humidity < HUMIDITY_SAND_THRESHOLD)
						{
							SetBlockType(localX, localY, localZ, "sand");
						}
						else if (humidity > HUMIDITY_SAND_THRESHOLD && humidity < HUMIDITY_BEACH_THRESHOLD && localZ == SEA_LEVEL)
						{
							SetBlockType(localX, localY, localZ, "sand");
						}
						else
						{
							SetBlockType(localX, localY, localZ, "grass");
						}
					}
					//blocks between grass and stone are dirt
					else if (localZ < terrainHeightZ && localZ >= stoneHeightZ)
					{
						if (humidity < HUMIDITY_SAND_THRESHOLD && localZ >= terrainHeightZ - sandThickness)
						{
							SetBlockType(localX, localY, localZ, "sand");
						}
						else
						{
							SetBlockType(localX, localY, localZ, "dirt");
						}
					}
					//blocks below dirt are usually stone, occasionally ore
					else if (localZ < stoneHeightZ)
					{
						float oreChanceVar = Get3dNoiseZeroToOne(globalX, globalY, localZ, currentSeed++);

						if (oreChanceVar <= DIAMOND_RANGE_MAX)
						{
							SetBlockType(localX, localY, localZ, "diamond");
						}
						else if (oreChanceVar > DIAMOND_RANGE_MAX && oreChanceVar <= GOLD_RANGE_MAX)
						{
							SetBlockType(localX, localY, localZ, "gold");
						}
						else if (oreChanceVar > GOLD_RANGE_MAX && oreChanceVar <= IRON_RANGE_MAX)
						{
							SetBlockType(localX, localY, localZ, "iron");
						}
						else if (oreChanceVar > IRON_RANGE_MAX && oreChanceVar <= COAL_RANGE_MAX)
						{
							SetBlockType(localX, localY, localZ, "coal");
						}
						else
						{
							SetBlockType(localX, localY, localZ, "stone");
						}
					}
					else if (localZ > terrainHeightZ)
					{
						//place water at and under sea level (or ice if it's cold enough)
						if (localZ <= SEA_LEVEL)
						{
							if (temperature < TEMPERATURE_ICE_THRESHOLD && localZ >= SEA_LEVEL - iceThickness)
							{
								SetBlockType(localX, localY, localZ, "ice");
							}
							else
							{
								SetBlockType(localX, localY, localZ, "water");
							}
						}
						
					}
				}

				//generate trees
				if (localZ == terrainHeightZ + 1 && isHighestTreeNoiseInGrid && terrainHeightZ > SEA_LEVEL)
				{
					if (humidity < HUMIDITY_SAND_THRESHOLD)
					{
						blockTemplateStartingPositions.emplace_back("Cactus", IntVec3(localX, localY, localZ));
					}
					else if (temperature < TEMPERATURE_ICE_THRESHOLD)
					{
						blockTemplateStartingPositions.emplace_back("Spruce Tree", IntVec3(localX, localY, localZ));
					}
					else
					{
						blockTemplateStartingPositions.emplace_back("Oak Tree", IntVec3(localX, localY, localZ));
					}
				}

				//generate giant mushrooms
				if (localZ == terrainHeightZ && isHighestMushroomNoiseInGrid && terrainHeightZ > SEA_LEVEL)
				{
					if (humidity > HUMIDITY_MUSHROOM_THRESHOLD)
					{
						blockTemplateStartingPositions.emplace_back("Giant Mushroom", IntVec3(localX, localY, localZ));
					}
				}
			}
		}
	}

	//add caves
	AddCaves(currentSeed++, blockTemplateStartingPositions);

	//loop through all block templates that need to be spawned
	for (int templateIndex = 0; templateIndex < blockTemplateStartingPositions.size(); templateIndex++)
	{
		IntVec3& startingPos = blockTemplateStartingPositions[templateIndex].m_localBlockCoords;
		std::string& templateType = blockTemplateStartingPositions[templateIndex].m_blockName;
		
		BlockTemplate blockTemplate;
		if (templateType == "Cactus")
		{
			blockTemplate = *BlockTemplate::GetBlockTemplateByName("Cactus");
		}
		else if (templateType == "Oak Tree")
		{
			blockTemplate = *BlockTemplate::GetBlockTemplateByName("Oak Tree");
		}
		else if (templateType == "Spruce Tree")
		{
			blockTemplate = *BlockTemplate::GetBlockTemplateByName("Spruce Tree");
		}
		else if (templateType == "Lava Pit")
		{
			blockTemplate = *BlockTemplate::GetBlockTemplateByName("Lava Pit");
		}
		else if (templateType == "Giant Mushroom")
		{
			blockTemplate = *BlockTemplate::GetBlockTemplateByName("Giant Mushroom");
		}

		//loop through each block and place it if it's within the chunk
		for (int blueprintIndex = 0; blueprintIndex < blockTemplate.m_blueprint.size(); blueprintIndex++)
		{
			BlockTemplateEntry& blueprintEntry = blockTemplate.m_blueprint[blueprintIndex];
			
			int localX = startingPos.x + blueprintEntry.m_localBlockCoords.x;
			int localY = startingPos.y + blueprintEntry.m_localBlockCoords.y;
			int localZ = startingPos.z + blueprintEntry.m_localBlockCoords.z;

			if (localX >= 0 && localX < CHUNK_SIZE_X && localY >= 0 && localY < CHUNK_SIZE_Y && localZ >= 0 && localZ < CHUNK_SIZE_Z)
			{
				SetBlockType(localX, localY, localZ, blueprintEntry.m_blockName);
			}
		}
	}

	m_needsSaving = false;	//we don't need to save if we just generated this chunk
}


void Chunk::AddCaves(unsigned int worldCaveSeed, std::vector<BlockTemplateEntry>& blockTemplateOrigins)
{
	std::vector<IntVec2> caveStartingChunks;	//keeps track of which chunk coords have a cave
	
	//get cave noise values for large surrounding area
	for (int chunkY = m_chunkCoords.y - CAVE_MAX_CHUNK_RADIUS; chunkY <= m_chunkCoords.y + CAVE_MAX_CHUNK_RADIUS; chunkY++)
	{
		for (int chunkX = m_chunkCoords.x - CAVE_MAX_CHUNK_RADIUS; chunkX <= m_chunkCoords.x + CAVE_MAX_CHUNK_RADIUS; chunkX++)
		{
			float caveStartNoise = Get2dNoiseZeroToOne(chunkX, chunkY, worldCaveSeed);

			if (caveStartNoise < CAVE_GENERATION_CHANCE)
			{
				caveStartingChunks.emplace_back(chunkX, chunkY);
			}
		}
	}

	//now actually process vector of chunks where caves start in order to make caves
	for (int chunkIndex = 0; chunkIndex < caveStartingChunks.size(); chunkIndex++)
	{
		IntVec2 caveChunkCoords = caveStartingChunks[chunkIndex];

		//get fairly unique chunk cave seed using chunk's x and y coordinates
		unsigned int chunkCaveSeed = static_cast<unsigned int>(caveChunkCoords.x + caveChunkCoords.y * 357239);	//multiple y coord by large prime to make seed unique to coords

		RandomNumberGenerator caveRNG;
		caveRNG.SeedRNG(chunkCaveSeed);

		int numCaveSegments = caveRNG.RollRandomIntInRange(CAVE_MIN_SEGMENTS, CAVE_MAX_SEGMENTS);

		//decide origin block of cave within chunk
		int localCaveOriginX = caveRNG.RollRandomIntLessThan(CHUNK_SIZE_X);
		int localCaveOriginY = caveRNG.RollRandomIntLessThan(CHUNK_SIZE_Y);
		int localCaveOriginZ = caveRNG.RollRandomIntInRange(CAVE_ORIGIN_MIN_Z, CAVE_ORIGIN_MAX_Z);

		int globalX = localCaveOriginX + (caveChunkCoords.x * CHUNK_SIZE_X);
		int globalY = localCaveOriginY + (caveChunkCoords.y * CHUNK_SIZE_Y);

		//determine range of each section
		Vec3 segmentStart = Vec3(static_cast<float>(globalX), static_cast<float>(globalY), static_cast<float>(localCaveOriginZ));
		float horizontalDirectionDegrees = 0.0f;

		for (int segmentIndex = 0; segmentIndex < numCaveSegments; segmentIndex++)
		{
			//determine direction to wander in
			float horizontalDirectionChange = Compute3dPerlinNoise(segmentStart.x, segmentStart.y, segmentStart.z, 1.0f, 3, 0.5f, 0.2f, true, chunkCaveSeed);
			horizontalDirectionChange = RangeMap(horizontalDirectionChange, -1.0f, 1.0f, -CAVE_MAX_ANGLE_CHANGE, CAVE_MAX_ANGLE_CHANGE);
			horizontalDirectionDegrees += horizontalDirectionChange;

			//determine how long to wander in that direction
			float segmentLength = static_cast<float>(caveRNG.RollRandomIntInRange(CAVE_SEGMENT_MIN_LENGTH, CAVE_SEGMENT_MAX_LENGTH));
			
			//make 2D vector pointing in that direction in that length
			Vec2 segmentDirection = Vec2::MakeFromPolarDegrees(horizontalDirectionDegrees, segmentLength);

			float heightChange = CAVE_MAX_HEIGHT_CHANGE * Compute3dPerlinNoise(segmentStart.x, segmentStart.y, segmentStart.z, 1.0f, 3, 0.5f, 2.0f, true, chunkCaveSeed);

			//segment end point is the starting point plus the 2D vector
			Vec3 segmentEnd = segmentStart + Vec3(segmentDirection.x, segmentDirection.y, heightChange);

			//only carve blocks for caves that actually touch chunk
			Vec2 chunkCenter = (Vec2(m_bounds.m_mins.x, m_bounds.m_mins.y) + Vec2(m_bounds.m_maxs.x, m_bounds.m_maxs.y)) * 0.5f;
			Vec2 segmentClosestPointXY = GetNearestPointOnCapsule2D(chunkCenter, Vec2(segmentStart.x, segmentStart.y), Vec2(segmentEnd.x, segmentEnd.y), CAVE_MAX_RADIUS);
			AABB2 chunkBoundsXY = AABB2(m_bounds.m_mins.x, m_bounds.m_mins.y, m_bounds.m_maxs.x, m_bounds.m_maxs.y);

			if (IsPointInsideAABB2D(segmentClosestPointXY, chunkBoundsXY))
			{
				//actually start carving blocks in same way as block templates if we get here
				float segmentRadius = SmoothStep3(Compute3dPerlinNoise(segmentStart.x, segmentStart.y, segmentStart.z, 0.75f, 1, 0.5f, 2.0f, true, chunkCaveSeed));
				segmentRadius = RangeMapClamped(segmentRadius, -0.8f, 0.8f, CAVE_MIN_RADIUS, CAVE_MAX_RADIUS);

				for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
				{
					BlockIterator iter = BlockIterator(blockIndex, this);
					AABB3 blockBounds = iter.GetBlockBounds();

					Vec3 nearestPointToSegment = GetNearestPointOnCapsule3D(iter.GetWorldCenter(), segmentStart, segmentEnd, segmentRadius);
					if (IsPointInsideAABB3D(nearestPointToSegment, blockBounds))
					{
						std::string blockType = GetBlockType(blockIndex);
						if (blockType != "water" && blockType != "ice")	//caves can't carve oceans
						{
							SetBlockType(blockIndex, "air");
						}
					}
				}
			}

			//check for lava pools
			float volcanicness = 0.5f + 0.5f * Compute3dPerlinNoise(segmentStart.x, segmentStart.y, segmentStart.z, 1.0f, 5, 0.5f, 2.0f, true, chunkCaveSeed + 1);
			if (volcanicness >= LAVA_PIT_THRESHOLD)
			{
				int localSegmentStartX = static_cast<int>(segmentStart.x) - (m_chunkCoords.x * CHUNK_SIZE_X);
				int localSegmentStartY = static_cast<int>(segmentStart.y) - (m_chunkCoords.y * CHUNK_SIZE_Y);
				blockTemplateOrigins.emplace_back("Lava Pit", IntVec3(localSegmentStartX, localSegmentStartY, static_cast<int>(segmentStart.z) - CAVE_MAX_RADIUS));
			}

			segmentStart = segmentEnd;
		}
	}
}


void Chunk::SetBlockType(int blockX, int blockY, int blockZ, std::string blockName)
{
	int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
	SetBlockType(blockIndex, blockName);
}


void Chunk::SetBlockType(int blockIndex, std::string blockName)
{
	m_blocks[blockIndex].m_blockType = static_cast<uint8_t>(BlockDefinition::GetBlockDefIDFromName(blockName));
	SetVertsAsDirty();
	m_needsSaving = true;
}


void Chunk::SetBlockType(int blockIndex, uint8_t blockDefID)
{
	m_blocks[blockIndex].m_blockType = blockDefID;
	SetVertsAsDirty();
	m_needsSaving = true;
}


void Chunk::SetBlockIsSky(int blockX, int blockY, int blockZ)
{
	int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	m_blocks[blockIndex].SetIsSky(true);
}


std::string Chunk::GetBlockType(int blockX, int blockY, int blockZ) const
{
	int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
	int blockDefIndex = m_blocks[blockIndex].m_blockType;

	return BlockDefinition::s_blockDefs[blockDefIndex].m_name;
}


std::string Chunk::GetBlockType(int blockIndex) const
{
	int blockDefIndex = m_blocks[blockIndex].m_blockType;

	return BlockDefinition::s_blockDefs[blockDefIndex].m_name;
}


bool Chunk::IsBlockOpaque(int blockX, int blockY, int blockZ) const
{
	int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
	
	return IsBlockOpaque(blockIndex);
}


bool Chunk::IsBlockOpaque(int blockIndex) const
{
	int blockDefIndex = m_blocks[blockIndex].m_blockType;

	return BlockDefinition::s_blockDefs[blockDefIndex].m_isOpaque;
}


int Chunk::GetBlockLightEmissionValue(int blockIndex) const
{
	int blockDefIndex = m_blocks[blockIndex].m_blockType;

	return BlockDefinition::s_blockDefs[blockDefIndex].m_lightEmissionValue;
}


bool Chunk::SaveChunk()
{
	m_needsSaving = false;
	
	std::vector<uint8_t> chunkBuffer;

	//save header
	chunkBuffer.emplace_back(static_cast<uint8_t>('G'));
	chunkBuffer.emplace_back(static_cast<uint8_t>('C'));
	chunkBuffer.emplace_back(static_cast<uint8_t>('H'));
	chunkBuffer.emplace_back(static_cast<uint8_t>('K'));
	chunkBuffer.emplace_back(static_cast<uint8_t>(3));
	chunkBuffer.emplace_back(static_cast<uint8_t>(CHUNK_BITS_X));
	chunkBuffer.emplace_back(static_cast<uint8_t>(CHUNK_BITS_Y));
	chunkBuffer.emplace_back(static_cast<uint8_t>(CHUNK_BITS_Z));

	//save world seed
	uint8_t worldSeedByte1 = static_cast<uint8_t>(m_world->m_worldSeed >> 24);
	uint8_t worldSeedByte2 = static_cast<uint8_t>(m_world->m_worldSeed >> 16);	//should automatically cut off higher bits
	uint8_t worldSeedByte3 = static_cast<uint8_t>(m_world->m_worldSeed >> 8);
	uint8_t worldSeedByte4 = static_cast<uint8_t>(m_world->m_worldSeed);

	chunkBuffer.emplace_back(worldSeedByte4);
	chunkBuffer.emplace_back(worldSeedByte3);
	chunkBuffer.emplace_back(worldSeedByte2);
	chunkBuffer.emplace_back(worldSeedByte1);

	//save run length
	uint8_t currentBlockType = 0;
	uint8_t currentBlockRunLength = 0;
	int totalRunLength = 0;
	for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
	{
		Block& block = m_blocks[blockIndex];

		//initialize current block type when starting out
		if (blockIndex == 0)
		{
			currentBlockType = block.m_blockType;
		}
		else
		{
			//if block type changes, save out previous block type and run length
			if (block.m_blockType != currentBlockType)
			{
				chunkBuffer.emplace_back(currentBlockType);
				chunkBuffer.emplace_back(currentBlockRunLength);

				currentBlockType = block.m_blockType;
				currentBlockRunLength = 0;
			}
			else
			{
				//if at final block or if run length has reached 255, go ahead and save the run length
				if (blockIndex == CHUNK_TOTAL_BLOCKS - 1 || currentBlockRunLength == 255)
				{
					chunkBuffer.emplace_back(currentBlockType);
					chunkBuffer.emplace_back(currentBlockRunLength);

					currentBlockRunLength = 0;
				}
			}
		}

		currentBlockRunLength++;
		totalRunLength++;
	}

	if (totalRunLength != CHUNK_TOTAL_BLOCKS)
	{
		std::string errorText = Stringf("Chunk %i, %i didn't save enough data!", m_chunkCoords.x, m_chunkCoords.y);
		ERROR_AND_DIE(errorText.c_str());
	}


	
	std::string fileName = Stringf("Saves/World_%u/Chunk(%i,%i).chunk", m_world->m_worldSeed, m_chunkCoords.x, m_chunkCoords.y);

	return FileWriteFromBuffer(chunkBuffer, fileName);
}


void Chunk::LoadChunk()
{
	std::vector<uint8_t> chunkBuffer;

	std::string fileName = Stringf("Saves/World_%u/Chunk(%i,%i).chunk", m_world->m_worldSeed, m_chunkCoords.x, m_chunkCoords.y);

	FileReadToBuffer(chunkBuffer, fileName);

	//check header
	if (chunkBuffer[0] != 'G' || chunkBuffer[1] != 'C' || chunkBuffer[2] != 'H' || chunkBuffer[3] != 'K')
	{
		ERROR_AND_DIE("Chunk file missing 4CC!");
	}
	else if (chunkBuffer[4] != 3)
	{
		ERROR_AND_DIE("Chunk file version incorrect!");
	}
	else if (chunkBuffer[5] != CHUNK_BITS_X || chunkBuffer[6] != CHUNK_BITS_Y || chunkBuffer[7] != CHUNK_BITS_Z)
	{
		ERROR_AND_DIE("Chunk file specifies incorrect number of bits!");
	}

	//check seed
	unsigned int worldSeedByte1 = static_cast<unsigned int>(chunkBuffer[8]) << 24;
	unsigned int worldSeedByte2 = static_cast<unsigned int>(chunkBuffer[9]) << 16;
	unsigned int worldSeedByte3 = static_cast<unsigned int>(chunkBuffer[10]) << 8;
	unsigned int worldSeedByte4 = static_cast<unsigned int>(chunkBuffer[11]);
	unsigned int loadedWorldSeed = worldSeedByte4 | worldSeedByte3 | worldSeedByte2 | worldSeedByte1;

	//if seed doesn't match, generate from scratch instead
	if (loadedWorldSeed != m_world->m_worldSeed)
	{
		PopulateBlocks();
		return;
	}

	//load blocks
	int blockIndex = 0;
	int totalRunLength = 0;

	for (int bufferIndex = 12; bufferIndex < chunkBuffer.size() - 1; bufferIndex = bufferIndex + 2)
	{
		uint8_t nextBlockType = chunkBuffer[bufferIndex];
		uint8_t nextBlockRunLength = chunkBuffer[bufferIndex + 1];
		totalRunLength += nextBlockRunLength;

		if (totalRunLength > CHUNK_TOTAL_BLOCKS)
		{
			std::string errorText = Stringf("Chunk %i, %i save file has too much data!", m_chunkCoords.x, m_chunkCoords.y);
			ERROR_AND_DIE(errorText.c_str());
		}

		for (int runIndex = 0; runIndex < nextBlockRunLength; runIndex++)
		{

			SetBlockType(blockIndex, nextBlockType);
			blockIndex++;
		}
	}

	m_needsSaving = false;
}


int Chunk::GetBlockIndexFromLocalCoords(int localX, int localY, int localZ) const
{
	return localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
}


Vec3 Chunk::GetChunkCenter() const
{
	return Vec3(m_bounds.m_mins.x + CHUNK_SIZE_X * 0.5f, m_bounds.m_mins.y + CHUNK_SIZE_Y * 0.5f, m_bounds.m_mins.z + CHUNK_SIZE_Z * 0.5f);
}


void Chunk::SetVertsAsDirty()
{
	m_areVertsDirty = true;
}


//
//private rendering functions
//
void Chunk::RebuildVertexes()
{
	m_cpuMesh.clear();
	for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
	{
		AddVertsForBlock(m_cpuMesh, blockIndex);
	}

	g_theRenderer->CopyCPUToGPU(m_cpuMesh.data(), m_cpuMesh.size() * sizeof(Vertex_PCU), m_gpuMesh);

	m_areVertsDirty = false;
}


void Chunk::AddVertsForBlock(std::vector<Vertex_PCU>& verts, int blockIndex)
{
	BlockDefinition const* blockDef = BlockDefinition::GetBlockDefFromID(m_blocks[blockIndex].m_blockType);
	if (!blockDef->m_isVisible)
	{
		return;
	}
	
	int localX = blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	float globalXMin = static_cast<float>(localX + (m_chunkCoords.x * CHUNK_SIZE_X));
	float globalYMin = static_cast<float>(localY + (m_chunkCoords.y * CHUNK_SIZE_Y));
	float globalZMin = static_cast<float>(localZ);
	float globalXMax = globalXMin + 1.0f;
	float globalYMax = globalYMin + 1.0f;
	float globalZMax = globalZMin + 1.0f;

	AABB2 topUVs = g_worldSpriteSheet->GetSpriteUVs(blockDef->m_topSpriteIndex);
	AABB2 sideUVs = g_worldSpriteSheet->GetSpriteUVs(blockDef->m_sideSpriteIndex);
	AABB2 bottomUVs = g_worldSpriteSheet->GetSpriteUVs(blockDef->m_bottomSpriteIndex);

	Vec3 bottomLeftBack = Vec3(globalXMin, globalYMax, globalZMin);
	Vec3 bottomRightBack = Vec3(globalXMin, globalYMin, globalZMin);
	Vec3 topLeftBack = Vec3(globalXMin, globalYMax, globalZMax);
	Vec3 topRightBack = Vec3(globalXMin, globalYMin, globalZMax);
	Vec3 bottomLeftFront = Vec3(globalXMax, globalYMax, globalZMin);
	Vec3 bottomRightFront = Vec3(globalXMax, globalYMin, globalZMin);
	Vec3 topLeftFront = Vec3(globalXMax, globalYMax, globalZMax);
	Vec3 topRightFront = Vec3(globalXMax, globalYMin, globalZMax);

	bool drawWestFace = true;
	bool drawEastFace = true;
	bool drawSouthFace = true;
	bool drawNorthFace = true;
	bool drawDownwardFace = true;
	bool drawSkywardFace = true;

	BlockIterator thisBlock = BlockIterator(blockIndex, this);
	BlockIterator eastNeighbor = thisBlock.GetEastNeighbor();
	BlockIterator westNeighbor = thisBlock.GetWestNeighbor();
	BlockIterator northNeighbor = thisBlock.GetNorthNeighbor();
	BlockIterator southNeighbor = thisBlock.GetSouthNeighbor();
	BlockIterator skywardNeighbor = thisBlock.GetSkywardNeighbor();
	BlockIterator downwardNeighbor = thisBlock.GetDownwardNeighbor();

	if (g_enableHiddenSurfaceRemoval)	//only bother with checking surrounding blocks if hidden surface removal is turned on
	{
		//check west (-x) block
		if (westNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* westBlockDef = BlockDefinition::GetBlockDefFromID(westNeighbor.GetBlock()->m_blockType);
			if (westBlockDef->m_isOpaque)
			{
				drawWestFace = false;
			}
		}

		//check east (+x) block
		if (eastNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* eastBlockDef = BlockDefinition::GetBlockDefFromID(eastNeighbor.GetBlock()->m_blockType);
			if (eastBlockDef->m_isOpaque)
			{
				drawEastFace = false;
			}
		}

		//check south (-y) block
		if (southNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* southBlockDef = BlockDefinition::GetBlockDefFromID(southNeighbor.GetBlock()->m_blockType);
			if (southBlockDef->m_isOpaque)
			{
				drawSouthFace = false;
			}
		}

		//check north (+y) block
		if (northNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* northBlockDef = BlockDefinition::GetBlockDefFromID(northNeighbor.GetBlock()->m_blockType);
			if (northBlockDef->m_isOpaque)
			{
				drawNorthFace = false;
			}
		}

		//check downward (-z) block
		if (downwardNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* downwardBlockDef = BlockDefinition::GetBlockDefFromID(downwardNeighbor.GetBlock()->m_blockType);
			if (downwardBlockDef->m_isOpaque)
			{
				drawDownwardFace = false;
			}
		}

		//check skyward (+z) block
		if (skywardNeighbor.GetChunk() != nullptr)
		{
			BlockDefinition const* skywardBlockDef = BlockDefinition::GetBlockDefFromID(skywardNeighbor.GetBlock()->m_blockType);
			if (skywardBlockDef->m_isOpaque)
			{
				drawSkywardFace = false;
			}
		}
	}

	//get light values of surrounding blocks
	unsigned char westOutdoorLightValue = 0;
	unsigned char westIndoorLightValue = 0;
	unsigned char eastOutdoorLightValue = 0;
	unsigned char eastIndoorLightValue = 0;
	unsigned char northOutdoorLightValue = 0;
	unsigned char northIndoorLightValue = 0;
	unsigned char southOutdoorLightValue = 0;
	unsigned char southIndoorLightValue = 0;
	unsigned char skywardOutdoorLightValue = 0;
	unsigned char skywardIndoorLightValue = 0;
	unsigned char downwardOutdoorLightValue = 0;
	unsigned char downwardIndoorLightValue = 0;

	if (eastNeighbor.GetChunk() != nullptr && drawEastFace)
	{
		eastOutdoorLightValue = eastNeighbor.GetBlock()->GetOutdoorLightLevel();
		float eastOutdoorLightValueNormalized = NormalizeByte(eastOutdoorLightValue);
		eastOutdoorLightValueNormalized = RangeMapClamped(eastOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		eastOutdoorLightValue = DenormalizeByte(eastOutdoorLightValueNormalized);

		eastIndoorLightValue = eastNeighbor.GetBlock()->GetIndoorLightLevel();
		float eastIndoorLightValueNormalized = NormalizeByte(eastIndoorLightValue);
		eastIndoorLightValueNormalized = RangeMapClamped(eastIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		eastIndoorLightValue = DenormalizeByte(eastIndoorLightValueNormalized);
	}
	if (westNeighbor.GetChunk() != nullptr && drawWestFace)
	{
		westOutdoorLightValue = westNeighbor.GetBlock()->GetOutdoorLightLevel();
		float westOutdoorLightValueNormalized = NormalizeByte(westOutdoorLightValue);
		westOutdoorLightValueNormalized = RangeMapClamped(westOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		westOutdoorLightValue = DenormalizeByte(westOutdoorLightValueNormalized);

		westIndoorLightValue = westNeighbor.GetBlock()->GetIndoorLightLevel();
		float westIndoorLightValueNormalized = NormalizeByte(westIndoorLightValue);
		westIndoorLightValueNormalized = RangeMapClamped(westIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		westIndoorLightValue = DenormalizeByte(westIndoorLightValueNormalized);
	}
	if (northNeighbor.GetChunk() != nullptr && drawNorthFace)
	{
		northOutdoorLightValue = northNeighbor.GetBlock()->GetOutdoorLightLevel();
		float northOutdoorLightValueNormalized = NormalizeByte(northOutdoorLightValue);
		northOutdoorLightValueNormalized = RangeMapClamped(northOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		northOutdoorLightValue = DenormalizeByte(northOutdoorLightValueNormalized);

		northIndoorLightValue = northNeighbor.GetBlock()->GetIndoorLightLevel();
		float northIndoorLightValueNormalized = NormalizeByte(northIndoorLightValue);
		northIndoorLightValueNormalized = RangeMapClamped(northIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		northIndoorLightValue = DenormalizeByte(northIndoorLightValueNormalized);
	}
	if (southNeighbor.GetChunk() != nullptr && drawSouthFace)
	{
		southOutdoorLightValue = southNeighbor.GetBlock()->GetOutdoorLightLevel();
		float southOutdoorLightValueNormalized = NormalizeByte(southOutdoorLightValue);
		southOutdoorLightValueNormalized = RangeMapClamped(southOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		southOutdoorLightValue = DenormalizeByte(southOutdoorLightValueNormalized);

		southIndoorLightValue = southNeighbor.GetBlock()->GetIndoorLightLevel();
		float southIndoorLightValueNormalized = NormalizeByte(southIndoorLightValue);
		southIndoorLightValueNormalized = RangeMapClamped(southIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		southIndoorLightValue = DenormalizeByte(southIndoorLightValueNormalized);
	}
	if (skywardNeighbor.GetChunk() != nullptr && drawSkywardFace)
	{
		skywardOutdoorLightValue = skywardNeighbor.GetBlock()->GetOutdoorLightLevel();
		float skywardOutdoorLightValueNormalized = NormalizeByte(skywardOutdoorLightValue);
		skywardOutdoorLightValueNormalized = RangeMapClamped(skywardOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		skywardOutdoorLightValue = DenormalizeByte(skywardOutdoorLightValueNormalized);

		skywardIndoorLightValue = skywardNeighbor.GetBlock()->GetIndoorLightLevel();
		float skywardIndoorLightValueNormalized = NormalizeByte(skywardIndoorLightValue);
		skywardIndoorLightValueNormalized = RangeMapClamped(skywardIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		skywardIndoorLightValue = DenormalizeByte(skywardIndoorLightValueNormalized);
	}
	if (downwardNeighbor.GetChunk() != nullptr && drawDownwardFace)
	{
		downwardOutdoorLightValue = downwardNeighbor.GetBlock()->GetOutdoorLightLevel();
		float downwardOutdoorLightValueNormalized = NormalizeByte(downwardOutdoorLightValue);
		downwardOutdoorLightValueNormalized = RangeMapClamped(downwardOutdoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		downwardOutdoorLightValue = DenormalizeByte(downwardOutdoorLightValueNormalized);

		downwardIndoorLightValue = downwardNeighbor.GetBlock()->GetIndoorLightLevel();
		float downwardIndoorLightValueNormalized = NormalizeByte(downwardIndoorLightValue);
		downwardIndoorLightValueNormalized = RangeMapClamped(downwardIndoorLightValueNormalized, 0.0f, (15.0f / 255.0f), 0.0f, 1.0f);
		downwardIndoorLightValue = DenormalizeByte(downwardIndoorLightValueNormalized);
	}
	
	if (drawEastFace)
	{
		AddVertsForQuad3D(verts, bottomRightFront, bottomLeftFront, topRightFront, topLeftFront, Rgba8(eastOutdoorLightValue, eastIndoorLightValue, 255), sideUVs);	//+x (east) face
	}
	if (drawWestFace)
	{
		AddVertsForQuad3D(verts, bottomLeftBack, bottomRightBack, topLeftBack, topRightBack, Rgba8(westOutdoorLightValue, westIndoorLightValue, 255), sideUVs);	//-x (west) face
	}
	if (drawNorthFace)
	{
		AddVertsForQuad3D(verts, bottomLeftFront, bottomLeftBack, topLeftFront, topLeftBack, Rgba8(northOutdoorLightValue, northIndoorLightValue, 255), sideUVs);	//+y (north) face
	}
	if (drawSouthFace)
	{
		AddVertsForQuad3D(verts, bottomRightBack, bottomRightFront, topRightBack, topRightFront, Rgba8(southOutdoorLightValue, southIndoorLightValue, 255), sideUVs);	//-y (south) face
	}
	if (drawSkywardFace)
	{
		AddVertsForQuad3D(verts, topLeftBack, topRightBack, topLeftFront, topRightFront, Rgba8(skywardOutdoorLightValue, skywardIndoorLightValue, 255), topUVs);	//+z (skyward) face
	}
	if (drawDownwardFace)
	{
		AddVertsForQuad3D(verts, bottomLeftFront, bottomRightFront, bottomLeftBack, bottomRightBack, Rgba8(downwardOutdoorLightValue, downwardIndoorLightValue, 255), bottomUVs);	//-z (downward) face
	}
}
