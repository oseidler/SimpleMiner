#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/Chunk.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/Player.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ChunkGenerateJob.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"
#include <windows.h>


//
//constructor and destructor
//
World::World(Game* owner)
	: m_game(owner)
{
	m_player = new Player(owner);
	m_player->m_position = Vec3(0.0f, 0.0f, 70.0f);

	m_worldSeed = g_gameConfigBlackboard.GetValue("worldSeed", m_worldSeed);

	CreateDirectoryA("Saves", NULL);
	std::string worldFolderPath = Stringf("Saves\\World_%u", m_worldSeed);
	CreateDirectoryA(worldFolderPath.c_str(), NULL);
}


World::~World()
{
	for (auto chunkIndex = m_activeChunks.begin(); chunkIndex != m_activeChunks.end(); chunkIndex++)
	{
		delete chunkIndex->second;
	}
	
	delete m_player;
}


//
//public game flow functions
//
void World::Update()
{
	//check for completed chunk generate jobs to retrieve
	while (g_theJobSystem->AreThereCompletedJobs())
	{
		ChunkGenerateJob* completedJob = dynamic_cast<ChunkGenerateJob*>(g_theJobSystem->ClaimCompletedJob());
		if (completedJob != nullptr)
		{
			ActivateChunk(completedJob->m_chunk->m_chunkCoords, completedJob->m_chunk);
			delete completedJob;
		}
	}
	
	float currentWorldTimeScale = m_worldTimeScale;

	//lock or unlock raycast
	if (g_theInput->WasKeyJustPressed('R'))
	{
		m_lockRaycast = !m_lockRaycast;
	}
	
	//update player position and orientation
	m_player->Update();

	//if we aren't locking raycast, match the raycast to the player
	if (!m_lockRaycast)
	{
		m_blockRaycastStart = m_player->m_position;
		m_blockRaycastDirection = m_player->GetModelMatrix().GetIBasis3D();
	}
	
	//debug key to disable world shader
	if (g_theInput->WasKeyJustPressed('L'))
	{
		m_usingWorldShader = !m_usingWorldShader;
	}
	//debug key to draw chunk bounds
	if (g_theInput->WasKeyJustPressed('M'))
	{
		m_drawPlayerChunkBoundaries = !m_drawPlayerChunkBoundaries;
	}

	//time acceleration debug key
	if (g_theInput->IsKeyDown('Y'))
	{
		currentWorldTimeScale *= 50.0f;
	}
	
	//debug key to deactivate all chunks
	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		DeactivateAllChunks();
	}

	//debug key to change seed
	if (g_theInput->WasKeyJustPressed(KEYCODE_F9))
	{
		DeactivateAllChunks();
		m_worldSeed++;
		std::string worldFolderPath = Stringf("Saves\\World_%u", m_worldSeed);
		CreateDirectoryA(worldFolderPath.c_str(), NULL);
	}

	//chunk activation logic
	static float chunkActivationDistance = g_gameConfigBlackboard.GetValue("chunkActivationDistance", 250.0f);
	static float chunkDeactivationDistance = chunkActivationDistance + static_cast<float>(CHUNK_SIZE_X + CHUNK_SIZE_Y);
	
	static int maxChunksRadiusX = 1 + (static_cast<int>(chunkActivationDistance) / CHUNK_SIZE_X);
	static int maxChunksRadiusY = 1 + (static_cast<int>(chunkActivationDistance) / CHUNK_SIZE_Y);
	static int maxChunks = 4 * maxChunksRadiusX * maxChunksRadiusY;

	if (m_queuedChunks.size() + m_activeChunks.size() < maxChunks)
	{
		IntVec2 missingChunkCoords = IntVec2();
		while (bool foundMissingChunk = FindNearestMissingChunk(missingChunkCoords, chunkActivationDistance))
		{
			if (foundMissingChunk)
			{
				Chunk* newChunk = new Chunk(missingChunkCoords, this);

				//check if chunk exists on disk here
				std::string fileName = Stringf("Saves//World_%u/Chunk(%i,%i).chunk", m_worldSeed, missingChunkCoords.x, missingChunkCoords.y);
				if (CheckForFile(fileName))
				{
					newChunk->LoadChunk();
					ActivateChunk(missingChunkCoords, newChunk);
				}
				else
				{
					//otherwise, post a job to populate its blocks
					ChunkGenerateJob* chunkJob = new ChunkGenerateJob(newChunk);
					g_theJobSystem->PostNewJob(chunkJob);
					m_queuedChunks.emplace(missingChunkCoords, newChunk);
				}
			}
		}
	}
	//check for any active chunks outside deactivation radius, and deactivate the farthest one found
	IntVec2 inactiveChunkCoords = IntVec2();
	while (bool foundInactiveChunk = FindFarthestInactiveChunk(inactiveChunkCoords, chunkDeactivationDistance))
	{
		if (foundInactiveChunk)
		{
			DeactivateChunk(inactiveChunkCoords);
		}
	}

	//do raycast for digging/placing
	GameRaycastResult3D blockRaycast = RaycastVsBlocks(m_blockRaycastStart, m_blockRaycastDirection, 8.0f);
	if (m_lockRaycast)
	{
		Vec3 rayEndPos = blockRaycast.m_rayStartPosition + (blockRaycast.m_rayDirection * blockRaycast.m_rayLength);

		DebugAddWorldLine(blockRaycast.m_rayStartPosition, rayEndPos, 0.025f, 0.0f, Rgba8(255, 0, 255), Rgba8(255, 0, 255));
		DebugAddWorldLine(blockRaycast.m_rayStartPosition, rayEndPos, 0.01f, 0.0f, Rgba8(255, 0, 255, 192), Rgba8(255, 0, 255, 192), DebugRenderMode::X_RAY);
		DebugAddWorldSphere(blockRaycast.m_impactPos, 0.1f, 0.0f, Rgba8(255, 0, 0), Rgba8(255, 0, 0));
	}
	if (blockRaycast.m_didImpact)
	{
		int blockIndex = blockRaycast.m_impactedBlock.GetBlockIndex();
		Chunk* chunk = blockRaycast.m_impactedBlock.GetChunk();
		
		int localX = blockIndex & (CHUNK_SIZE_X - 1);
		int localY = (blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
		int localZ = blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);
		
		float globalXMin = static_cast<float>(localX + (chunk->m_chunkCoords.x * CHUNK_SIZE_X));
		float globalYMin = static_cast<float>(localY + (chunk->m_chunkCoords.y * CHUNK_SIZE_Y));
		float globalZMin = static_cast<float>(localZ);
		float globalXMax = globalXMin + 1.0f;
		float globalYMax = globalYMin + 1.0f;
		float globalZMax = globalZMin + 1.0f;

		Vec3 bottomLeftBack = Vec3(globalXMin, globalYMax, globalZMin);
		Vec3 bottomRightBack = Vec3(globalXMin, globalYMin, globalZMin);
		Vec3 topLeftBack = Vec3(globalXMin, globalYMax, globalZMax);
		Vec3 topRightBack = Vec3(globalXMin, globalYMin, globalZMax);
		Vec3 bottomLeftFront = Vec3(globalXMax, globalYMax, globalZMin);
		Vec3 bottomRightFront = Vec3(globalXMax, globalYMin, globalZMin);
		Vec3 topLeftFront = Vec3(globalXMax, globalYMax, globalZMax);
		Vec3 topRightFront = Vec3(globalXMax, globalYMin, globalZMax);

		//if we hit east side
		if (blockRaycast.m_impactNormal == Vec3(1.0f, 0.0f, 0.0f))
		{
			DebugAddWorldLine(bottomRightFront, bottomLeftFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomRightFront, topRightFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftFront, topLeftFront, 0.01f, 0.0f);
			DebugAddWorldLine(topRightFront, topLeftFront, 0.01f, 0.0f);
		}
		//if we hit west side
		else if (blockRaycast.m_impactNormal == Vec3(-1.0f, 0.0f, 0.0f))
		{
			DebugAddWorldLine(bottomRightBack, bottomLeftBack, 0.01f, 0.0f);
			DebugAddWorldLine(bottomRightBack, topRightBack, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftBack, topLeftBack, 0.01f, 0.0f);
			DebugAddWorldLine(topRightBack, topLeftBack, 0.01f, 0.0f);
		}
		//if we hit north side
		else if (blockRaycast.m_impactNormal == Vec3(0.0f, 1.0f, 0.0f))
		{
			DebugAddWorldLine(bottomLeftFront, bottomLeftBack, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftFront, topLeftFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftBack, topLeftBack, 0.01f, 0.0f);
			DebugAddWorldLine(topLeftFront, topLeftBack, 0.01f, 0.0f);
		}
		//if we hit south side
		else if (blockRaycast.m_impactNormal == Vec3(0.0f, -1.0f, 0.0f))
		{
			DebugAddWorldLine(bottomRightFront, bottomRightBack, 0.01f, 0.0f);
			DebugAddWorldLine(bottomRightFront, topRightFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomRightBack, topRightBack, 0.01f, 0.0f);
			DebugAddWorldLine(topRightFront, topRightBack, 0.01f, 0.0f);
		}
		//if we hit skyward side
		else if (blockRaycast.m_impactNormal == Vec3(0.0f, 0.0f, 1.0f))
		{
			DebugAddWorldLine(topLeftBack, topRightBack, 0.01f, 0.0f);
			DebugAddWorldLine(topLeftBack, topLeftFront, 0.01f, 0.0f);
			DebugAddWorldLine(topRightBack, topRightFront, 0.01f, 0.0f);
			DebugAddWorldLine(topLeftFront, topRightFront, 0.01f, 0.0f);
		}
		//if we hit downward side
		else if (blockRaycast.m_impactNormal == Vec3(0.0f, 0.0f, -1.0f))
		{
			DebugAddWorldLine(bottomLeftBack, bottomRightBack, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftBack, bottomLeftFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomRightBack, bottomRightFront, 0.01f, 0.0f);
			DebugAddWorldLine(bottomLeftFront, bottomRightFront, 0.01f, 0.0f);
		}
		//if inside a block, render lines around all of it
		else if (blockRaycast.m_impactNormal == Vec3())
		{
			DebugAddWorldLine(bottomRightFront, bottomLeftFront, 0.001f, 0.0f);
			DebugAddWorldLine(bottomRightFront, topRightFront, 0.001f, 0.0f);
			DebugAddWorldLine(bottomLeftFront, topLeftFront, 0.001f, 0.0f);
			DebugAddWorldLine(topRightFront, topLeftFront, 0.001f, 0.0f);
			DebugAddWorldLine(bottomRightBack, bottomLeftBack, 0.001f, 0.0f);
			DebugAddWorldLine(bottomRightBack, topRightBack, 0.001f, 0.0f);
			DebugAddWorldLine(bottomLeftBack, topLeftBack, 0.001f, 0.0f);
			DebugAddWorldLine(topRightBack, topLeftBack, 0.001f, 0.0f);
			DebugAddWorldLine(topLeftBack, topLeftFront, 0.001f, 0.0f);
			DebugAddWorldLine(topRightBack, topRightFront, 0.001f, 0.0f);
			DebugAddWorldLine(bottomRightFront, bottomRightBack, 0.001f, 0.0f);
			DebugAddWorldLine(bottomLeftFront, bottomLeftBack, 0.001f, 0.0f);
		}
	}

	//digging and placing
	if (g_theInput->WasKeyJustPressed(KEYCODE_LMB))
	{
		Chunk* chunk = blockRaycast.m_impactedBlock.GetChunk();
		if (chunk != nullptr)
		{
			//set block to air and mark lighting as dirty
			int blockIndex = blockRaycast.m_impactedBlock.GetBlockIndex();
			chunk->SetBlockType(blockIndex, "air");
			MarkLightingDirty(chunk, blockIndex);

			//if block above is sky, descend downward and set each block to sky and mark lighting as dirty until hitting opaque block
			BlockIterator blockAbove = BlockIterator(blockIndex, chunk).GetSkywardNeighbor();
			if (blockAbove.GetBlock()->IsSky())
			{
				Block* block = &chunk->m_blocks[blockIndex];
				block->SetIsSky(true);
				BlockIterator nextBlockBelow = BlockIterator(blockIndex, chunk).GetDownwardNeighbor();

				while (nextBlockBelow.GetChunk() != nullptr && !chunk->IsBlockOpaque(nextBlockBelow.GetBlockIndex()))
				{
					nextBlockBelow.GetBlock()->SetIsSky(true);
					MarkLightingDirty(nextBlockBelow.GetChunk(), nextBlockBelow.GetBlockIndex());
					nextBlockBelow = nextBlockBelow.GetDownwardNeighbor();
				}
			}
		}
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RMB))
	{
		Chunk* chunk = blockRaycast.m_impactedBlock.GetChunk();
		if (chunk != nullptr)
		{
			BlockIterator blockIter = BlockIterator(-1, nullptr);
			
			if (blockRaycast.m_impactNormal == Vec3(1.0f, 0.0f, 0.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetEastNeighbor();
			}
			if (blockRaycast.m_impactNormal == Vec3(-1.0f, 0.0f, 0.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetWestNeighbor();
			}
			if (blockRaycast.m_impactNormal == Vec3(0.0f, 1.0f, 0.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetNorthNeighbor();
			}
			if (blockRaycast.m_impactNormal == Vec3(0.0f, -1.0f, 0.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetSouthNeighbor();
			}
			if (blockRaycast.m_impactNormal == Vec3(0.0f, 0.0f, 1.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetSkywardNeighbor();
			}
			if (blockRaycast.m_impactNormal == Vec3(0.0f, 0.0f, -1.0f))
			{
				blockIter = blockRaycast.m_impactedBlock.GetDownwardNeighbor();
			}

			Chunk* chunkOfPlacedBlock = blockIter.GetChunk();
			
			if (chunkOfPlacedBlock != nullptr)
			{
				//set block type and mark as dirty
				int blockIndex = blockIter.GetBlockIndex();
				chunkOfPlacedBlock->SetBlockType(blockIndex, m_player->m_blockIDToPlace);
				MarkLightingDirty(chunkOfPlacedBlock, blockIndex);

				//if block was sky, set it to no longer be sky, then clear all sky flags and flag dirty lighting directly below until hitting opaque
				BlockIterator thisBlock = BlockIterator(blockIndex, chunkOfPlacedBlock);
				if (thisBlock.GetBlock()->IsSky() && thisBlock.GetBlock()->IsOpaque())
				{
					thisBlock.GetBlock()->SetIsSky(false);

					BlockIterator nextBlockBelow = BlockIterator(blockIndex, chunkOfPlacedBlock).GetDownwardNeighbor();

					while (nextBlockBelow.GetChunk() != nullptr && !chunk->IsBlockOpaque(nextBlockBelow.GetBlockIndex()))
					{
						nextBlockBelow.GetBlock()->SetIsSky(false);
						MarkLightingDirty(nextBlockBelow.GetChunk(), nextBlockBelow.GetBlockIndex());
						nextBlockBelow = nextBlockBelow.GetDownwardNeighbor();
					}
				}
			}
		}
	}

	//do lighting stuff
	ProcessDirtyLighting();
	
	//update chunks
	for (auto chunkIndex = m_activeChunks.begin(); chunkIndex != m_activeChunks.end(); chunkIndex++)
	{
		Chunk*& chunk = chunkIndex->second;
		if (chunk != nullptr)
		{
			chunk->Update();
		}
	}

	//update world time
	m_worldTime += (m_game->m_gameClock.GetDeltaSeconds() * currentWorldTimeScale) / (86400.0f);
	std::string timeMessage = Stringf("Current day/time: %.3f", m_worldTime);
	DebugAddMessage(timeMessage, 0.0f);

	//update lighting based on world time
	float timeOfDay = m_worldTime - static_cast<int>(m_worldTime);
	if (timeOfDay > TIME_DUSK || timeOfDay < TIME_DAWN)
	{
		m_currentSkyColor = m_nightSkyColor;
		m_currentOutdoorLightColor = m_nightOutdoorLightColor;
	}
	else if(timeOfDay >= TIME_DAWN && timeOfDay <= TIME_NOON)
	{
		float dawnToNoonRange = RangeMapClamped(timeOfDay, TIME_DAWN, TIME_NOON, 0.0f, 1.0f);
		m_currentSkyColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightSkyColor.r), static_cast<float>(m_daySkyColor.r), dawnToNoonRange));
		m_currentSkyColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightSkyColor.g), static_cast<float>(m_daySkyColor.g), dawnToNoonRange));
		m_currentSkyColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightSkyColor.b), static_cast<float>(m_daySkyColor.b), dawnToNoonRange));
		m_currentOutdoorLightColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightOutdoorLightColor.r), static_cast<float>(m_dayOutdoorLightColor.r), dawnToNoonRange));
		m_currentOutdoorLightColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightOutdoorLightColor.g), static_cast<float>(m_dayOutdoorLightColor.g), dawnToNoonRange));
		m_currentOutdoorLightColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_nightOutdoorLightColor.b), static_cast<float>(m_dayOutdoorLightColor.b), dawnToNoonRange));
	}
	else //if timeOfDay > TIME_NOON && timeOfDay <= TIME_DUSK
	{
		float noonToDusk = RangeMapClamped(timeOfDay, TIME_NOON, TIME_DUSK, 0.0f, 1.0f);
		m_currentSkyColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_daySkyColor.r), static_cast<float>(m_nightSkyColor.r), noonToDusk));
		m_currentSkyColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_daySkyColor.g), static_cast<float>(m_nightSkyColor.g), noonToDusk));
		m_currentSkyColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_daySkyColor.b), static_cast<float>(m_nightSkyColor.b), noonToDusk));
		m_currentOutdoorLightColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_dayOutdoorLightColor.r), static_cast<float>(m_nightOutdoorLightColor.r), noonToDusk));
		m_currentOutdoorLightColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_dayOutdoorLightColor.g), static_cast<float>(m_nightOutdoorLightColor.g), noonToDusk));
		m_currentOutdoorLightColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_dayOutdoorLightColor.b), static_cast<float>(m_nightOutdoorLightColor.b), noonToDusk));
	}

	//lightning 
	float lightningPerlin = Compute1dPerlinNoise(m_worldTime * 2.0f, 1.0f, 9, 0.5f, 4.0f);
	float lightningStrength = RangeMapClamped(lightningPerlin, 0.6f, 0.9f, 0.0f, 1.0f);
	m_currentSkyColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentSkyColor.r), 255.0f, lightningStrength));
	m_currentSkyColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentSkyColor.g), 255.0f, lightningStrength));
	m_currentSkyColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentSkyColor.b), 255.0f, lightningStrength));
	m_currentOutdoorLightColor.r = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentOutdoorLightColor.r), 255.0f, lightningStrength));
	m_currentOutdoorLightColor.g = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentOutdoorLightColor.g), 255.0f, lightningStrength));
	m_currentOutdoorLightColor.b = static_cast<unsigned char>(Interpolate(static_cast<float>(m_currentOutdoorLightColor.b), 255.0f, lightningStrength));

	
	//indoor light flicker
	float glowPerlin = Compute1dPerlinNoise(m_worldTime * 50.0f, 1.0f, 9, 0.5f, 5.0f);
	float glowStrength = RangeMapClamped(glowPerlin, -1.0f, 1.0f, 0.8f, 1.0f);
	m_currentIndoorLightColor.r = static_cast<unsigned char>(glowStrength * static_cast<float>(m_baseIndoorLightColor.r));
	m_currentIndoorLightColor.g = static_cast<unsigned char>(glowStrength * static_cast<float>(m_baseIndoorLightColor.g));
	m_currentIndoorLightColor.b = static_cast<unsigned char>(glowStrength * static_cast<float>(m_baseIndoorLightColor.b));
}


void World::Render() const
{
	//clear screen based on time
	//do lerp color calculation here
	g_theRenderer->ClearScreen(m_currentSkyColor);
	
	for (auto chunkIndex = m_activeChunks.begin(); chunkIndex != m_activeChunks.end(); chunkIndex++)
	{
		Chunk* const& chunk = chunkIndex->second;
		if (chunk != nullptr)
		{
			g_theRenderer->BindTexture(&g_worldSpriteSheet->GetTexture());
			m_game->SetShaderGameConstants(m_player->m_position, m_currentIndoorLightColor, m_currentOutdoorLightColor, m_currentSkyColor);
			if (m_usingWorldShader)
			{
				g_theRenderer->BindShader(g_worldShader);
			}
			else
			{
				g_theRenderer->BindShader(nullptr);
			}

			chunk->Render();

			//draw debug chunk boundaries
			if (m_drawPlayerChunkBoundaries && IsPointInsideAABB3D(m_player->m_position, chunk->m_bounds))
			{
				std::vector<Vertex_PCU> boundaryVerts;
				AddVertsForAABB3D(boundaryVerts, chunk->m_bounds);
				g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
				g_theRenderer->BindTexture(nullptr);
				g_theRenderer->BindShader(nullptr);
				g_theRenderer->DrawVertexArray(boundaryVerts);
			}
		}
	}

	std::string blockPlacingString = Stringf("Selected Block Type: %s", BlockDefinition::GetBlockDefNameFromID(m_player->m_blockIDToPlace).c_str());
	DebugAddMessage(blockPlacingString, 0.0f);
}


//
//public chunk management functions
//
bool World::FindNearestMissingChunk(IntVec2& out_chunkCoords, float activationRadius)
{
	Vec2 playerPositionXY = Vec2(m_player->m_position.x, m_player->m_position.y);

	//calculate neighborhood bounds
	int neighborhoodMinX = static_cast<int>(playerPositionXY.x - activationRadius) / CHUNK_SIZE_X;
	int neighborhoodMaxX = static_cast<int>(playerPositionXY.x + activationRadius) / CHUNK_SIZE_X;
	int neighborhoodMinY = static_cast<int>(playerPositionXY.y - activationRadius) / CHUNK_SIZE_Y;
	int neighborhoodMaxY = static_cast<int>(playerPositionXY.y + activationRadius) / CHUNK_SIZE_Y;

	Vec2 currentClosestChunkPos = Vec2(FLT_MAX, FLT_MAX);
	bool missingChunkFound = false;

	for (int chunkX = neighborhoodMinX; chunkX <= neighborhoodMaxX; chunkX++)
	{
		for (int chunkY = neighborhoodMinY; chunkY <= neighborhoodMaxY; chunkY++)
		{
			//get chunk bounds
			float chunkMinX = static_cast<float>(chunkX * CHUNK_SIZE_X);
			float chunkMaxX = chunkMinX + static_cast<float>(CHUNK_SIZE_X);
			float chunkMinY = static_cast<float>(chunkY * CHUNK_SIZE_Y);
			float chunkMaxY = chunkMinY + static_cast<float>(CHUNK_SIZE_Y);
			AABB2 chunkBoundsXY = AABB2(chunkMinX, chunkMinY, chunkMaxX, chunkMaxY);

			Vec2 nearestPointOnChunk = GetNearestPointOnAABB2D(playerPositionXY, chunkBoundsXY);

			//check if the closest part of the chunk bounds are within activation radius
			if (IsPointInsideDisc2D(nearestPointOnChunk, playerPositionXY, activationRadius))
			{
				auto activeChunkFound = m_activeChunks.find(IntVec2(chunkX, chunkY));
				auto queuedChunkFound = m_queuedChunks.find(IntVec2(chunkX, chunkY));
				if (activeChunkFound == m_activeChunks.end() && queuedChunkFound == m_queuedChunks.end())
				{
					//if so, check if it's the closest to the player
					Vec2 chunkOrigin = Vec2(chunkMinX, chunkMinY);
					if (GetDistanceSquared2D(playerPositionXY, chunkOrigin) < GetDistanceSquared2D(playerPositionXY, currentClosestChunkPos))
					{
						currentClosestChunkPos = chunkOrigin;
						out_chunkCoords = IntVec2(chunkX, chunkY);
						missingChunkFound = true;
					}
				}
			}
		}
	}
	
	return missingChunkFound;
}


void World::ActivateChunk(IntVec2 chunkCoords, Chunk* chunk)
{
	m_activeChunks.emplace(chunkCoords, chunk);
	m_queuedChunks.erase(chunkCoords);
	chunk->m_state = ChunkState::ACTIVATED;

	//add pointers to neighbors
	//east neighbor
	auto chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x + 1, chunkCoords.y));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* eastChunk = chunkFound->second;
		chunk->m_eastNeighbor = eastChunk;
		eastChunk->m_westNeighbor = chunk;
	}

	//west neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x - 1, chunkCoords.y));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* westChunk = chunkFound->second;
		chunk->m_westNeighbor = westChunk;
		westChunk->m_eastNeighbor = chunk;
	}

	//north neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x, chunkCoords.y + 1));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* northChunk = chunkFound->second;
		chunk->m_northNeighbor = northChunk;
		northChunk->m_southNeighbor = chunk;
	}

	//south neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x, chunkCoords.y - 1));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* southChunk = chunkFound->second;
		chunk->m_southNeighbor = southChunk;
		southChunk->m_northNeighbor = chunk;
	}

	//mark non-opaque boundary blocks as dirty
	for (int blockX = 0; blockX < CHUNK_SIZE_X; blockX++)
	{
		for (int blockZ = 0; blockZ < CHUNK_SIZE_Z; blockZ++)
		{
			int blockY = 0;

			if (!chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
				MarkLightingDirty(chunk, blockIndex);
			}

			blockY = CHUNK_MAX_Y;

			if (!chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
				MarkLightingDirty(chunk, blockIndex);
			}
		}
	}
	for (int blockY = 0; blockY < CHUNK_SIZE_Y; blockY++)
	{
		for (int blockZ = 0; blockZ < CHUNK_SIZE_Z; blockZ++)
		{
			int blockX = 0;

			if (!chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
				MarkLightingDirty(chunk, blockIndex);
			}

			blockX = CHUNK_MAX_X;

			if (!chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
				MarkLightingDirty(chunk, blockIndex);
			}
		}
	}

	//descend down each column and mark sky blocks
	for (int blockX = 0; blockX < CHUNK_SIZE_X; blockX++)
	{
		for (int blockY = 0; blockY < CHUNK_SIZE_Y; blockY++)
		{
			int blockZ = CHUNK_MAX_Z;
			
			while (blockZ >= 0 && !chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				chunk->SetBlockIsSky(blockX, blockY, blockZ);
				blockZ--;
			}
		}
	}

	//descend again and mark outdoor light influence on sky blocks and non-opaque neighbors
	for (int blockX = 0; blockX < CHUNK_SIZE_X; blockX++)
	{
		for (int blockY = 0; blockY < CHUNK_SIZE_Y; blockY++)
		{
			int blockZ = CHUNK_MAX_Z;

			while (blockZ >= 0 && !chunk->IsBlockOpaque(blockX, blockY, blockZ))
			{
				int blockIndex = blockX + (blockY << CHUNK_BITS_X) + (blockZ << (CHUNK_BITS_X + CHUNK_BITS_Y));
				
				//set block's outdoor light influence
				BlockIterator blockIter = BlockIterator(blockIndex, chunk);
				blockIter.GetBlock()->SetOutdoorLightLevel(15);
				
				//mark non-opaque, non-sky horizontal neighbors as dirty
				BlockIterator eastNeighbor = blockIter.GetEastNeighbor();
				BlockIterator westNeighbor = blockIter.GetWestNeighbor();
				BlockIterator northNeighbor = blockIter.GetNorthNeighbor();
				BlockIterator southNeighbor = blockIter.GetSouthNeighbor();

				if (eastNeighbor.GetChunk() != nullptr && !eastNeighbor.GetBlock()->IsOpaque() && !eastNeighbor.GetBlock()->IsSky())
				{
					MarkLightingDirty(eastNeighbor.GetChunk(), eastNeighbor.GetBlockIndex());
				}
				if (westNeighbor.GetChunk() != nullptr && !westNeighbor.GetBlock()->IsOpaque() && !westNeighbor.GetBlock()->IsSky())
				{
					MarkLightingDirty(westNeighbor.GetChunk(), westNeighbor.GetBlockIndex());
				}
				if (northNeighbor.GetChunk() != nullptr && !northNeighbor.GetBlock()->IsOpaque() && !northNeighbor.GetBlock()->IsSky())
				{
					MarkLightingDirty(northNeighbor.GetChunk(), northNeighbor.GetBlockIndex());
				}
				if (southNeighbor.GetChunk() != nullptr && !southNeighbor.GetBlock()->IsOpaque() && !southNeighbor.GetBlock()->IsSky())
				{
					MarkLightingDirty(southNeighbor.GetChunk(), southNeighbor.GetBlockIndex());
				}

				blockZ--;
			}
		}
	}

	//loop through all blocks and mark light-emitting blocks as dirty
	for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
	{
		if (chunk->GetBlockLightEmissionValue(blockIndex) > 0)
		{
			MarkLightingDirty(chunk, blockIndex);
		}
	}
}


bool World::FindFarthestInactiveChunk(IntVec2& out_chunkCoords, float deactivationRadius)
{
	Vec2 playerPositionXY = Vec2(m_player->m_position.x, m_player->m_position.y);

	bool inactiveChunkFound = false;
	Vec2 currentFarthestChunkPos = playerPositionXY;
	
	for (auto chunkIndex = m_activeChunks.begin(); chunkIndex != m_activeChunks.end(); chunkIndex++)
	{
		Chunk*& chunk = chunkIndex->second;
		if (chunk != nullptr)
		{
  			Vec2 chunkCenter = Vec2(chunk->m_bounds.m_mins.x + (static_cast<float>(CHUNK_SIZE_X) * 0.5f), chunk->m_bounds.m_mins.y + (static_cast<float>(CHUNK_SIZE_Y) * 0.5f));

			//if active chunk is outside deactivation radius
			if (!IsPointInsideDisc2D(chunkCenter, playerPositionXY, deactivationRadius))
			{
				//see if it's farther than the previously saved farthest chunk position
				if (GetDistanceSquared2D(playerPositionXY, chunkCenter) > GetDistanceSquared2D(playerPositionXY, currentFarthestChunkPos))
				{
					currentFarthestChunkPos = chunkCenter;
					out_chunkCoords = chunkIndex->first;
					inactiveChunkFound = true;
				}
			}
		}
	}
	
	return inactiveChunkFound;
}


void World::DeactivateChunk(IntVec2 chunkCoords)
{
	Chunk* deactivatedChunk = m_activeChunks[chunkCoords];

	//remove pointers to it from neighbors
	//east neighbor
	auto chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x + 1, chunkCoords.y));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* eastChunk = chunkFound->second;
		deactivatedChunk->m_eastNeighbor = nullptr;
		eastChunk->m_westNeighbor = nullptr;
	}

	//west neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x - 1, chunkCoords.y));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* westChunk = chunkFound->second;
		deactivatedChunk->m_westNeighbor = nullptr;
		westChunk->m_eastNeighbor = nullptr;
	}

	//north neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x, chunkCoords.y + 1));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* northChunk = chunkFound->second;
		deactivatedChunk->m_northNeighbor = nullptr;
		northChunk->m_southNeighbor = nullptr;
	}

	//south neighbor
	chunkFound = m_activeChunks.find(IntVec2(chunkCoords.x, chunkCoords.y - 1));
	if (chunkFound != m_activeChunks.end())
	{
		Chunk* southChunk = chunkFound->second;
		deactivatedChunk->m_southNeighbor = nullptr;
		southChunk->m_northNeighbor = nullptr;
	}
	
	m_activeChunks.erase(chunkCoords);

	//do saving here
	if (deactivatedChunk->m_needsSaving)
	{
		deactivatedChunk->SaveChunk();
	}

	delete deactivatedChunk;
}


void World::DeactivateAllChunks()
{
	for (auto chunkIndex = m_activeChunks.begin(); chunkIndex != m_activeChunks.end(); chunkIndex++)
	{
		//gonna deactivate a little differently here because the normal way breaks the for loop using the iterator
		
		Chunk* deactivatedChunk = chunkIndex->second;

		//do same saving thing here as in normal deactivate chunk function
		if (deactivatedChunk->m_needsSaving)
		{
			deactivatedChunk->SaveChunk();
		}

		if (deactivatedChunk != nullptr)
		{
			delete deactivatedChunk;
			deactivatedChunk = nullptr;
		}
	}

	m_activeChunks.clear();
}


//
//public raycast functions
//
GameRaycastResult3D World::RaycastVsBlocks(Vec3 const& startPosition, Vec3 const& directionNormal, float distance)
{
	//go ahead and set up raycast result
	GameRaycastResult3D raycastResult;
	raycastResult.m_rayStartPosition = startPosition;
	raycastResult.m_rayDirection = directionNormal;
	raycastResult.m_rayLength = distance;
	
	//get world coordinates of block raycast starts in
	int worldX = static_cast<int>(floorf(startPosition.x));
	int worldY = static_cast<int>(floorf(startPosition.y));
	int worldZ = static_cast<int>(floorf(startPosition.z));

	//get chunk coordinates from world coordinates
	int chunkX = worldX >> CHUNK_BITS_X;
	int chunkY = worldY >> CHUNK_BITS_Y;
	
	//get chunk you're in (or if you aren't in an active chunk somehow, just don't bother)
	auto chunkFound = m_activeChunks.find(IntVec2(chunkX, chunkY));
	if (chunkFound == m_activeChunks.end())
	{
		return GameRaycastResult3D();
	}
	Chunk* chunk = chunkFound->second;

	//get local block coordinates
	int localX = worldX - (chunkX * CHUNK_SIZE_X);
	int localY = worldY - (chunkY * CHUNK_SIZE_Y);

	//get block index
	int blockIndex = localX + (localY << CHUNK_BITS_X) + (worldZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	//return blank raycast if above or below active chunk
	if (blockIndex < 0 || blockIndex >= CHUNK_TOTAL_BLOCKS)
	{
		return raycastResult;
	}

	//create block iterator for block we're in
	BlockIterator blockIter = BlockIterator(blockIndex, chunk);

	//return blank raycast if not in an active chunk
	if (blockIter.GetChunk() == nullptr)
	{
		return raycastResult;
	}

	//if we're already in a solid block, we're done
	if (blockIter.GetBlock()->IsSolid())
	{
		raycastResult.m_impactedBlock = blockIter;
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = 0.0f;
		raycastResult.m_impactPos = startPosition;
		raycastResult.m_impactNormal = Vec3();
		return raycastResult;
	}

	//initialization step
	float distPerXCrossing = 1.0f / (fabsf(directionNormal.x));
	int stepDirectionX;
	if (directionNormal.x < 0) stepDirectionX = -1;
	else stepDirectionX = 1;
	float xAtFirstXCrossing = static_cast<float>(worldX + (stepDirectionX + 1) / 2);
	float xDistToFirstXCrossing = xAtFirstXCrossing - startPosition.x;
	float totalDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * distPerXCrossing;

	float distPerYCrossing = 1.0f / (fabsf(directionNormal.y));
	int stepDirectionY;
	if (directionNormal.y < 0) stepDirectionY = -1;
	else stepDirectionY = 1;
	float yAtFirstYCrossing = static_cast<float>(worldY + (stepDirectionY + 1) / 2);
	float yDistToFirstYCrossing = yAtFirstYCrossing - startPosition.y;
	float totalDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * distPerYCrossing;

	float distPerZCrossing = 1.0f / (fabsf(directionNormal.z));
	int stepDirectionZ;
	if (directionNormal.z < 0) stepDirectionZ = -1;
	else stepDirectionZ = 1;
	float zAtFirstZCrossing = static_cast<float>(worldZ + (stepDirectionZ + 1) / 2);
	float zDistToFirstZCrossing = zAtFirstZCrossing - startPosition.z;
	float totalDistAtNextZCrossing = fabsf(zDistToFirstZCrossing) * distPerZCrossing;

	//main raycast loop
	while (totalDistAtNextXCrossing < distance || totalDistAtNextYCrossing < distance || totalDistAtNextZCrossing < distance)
	{
		if (totalDistAtNextXCrossing <= totalDistAtNextYCrossing && totalDistAtNextXCrossing <= totalDistAtNextZCrossing)
		{
			//return blank raycast if we surpass max raycast distance
			if (totalDistAtNextXCrossing > distance)
			{
				return raycastResult;
			}

			if (stepDirectionX == 1) blockIter = blockIter.GetEastNeighbor();
			else if (stepDirectionX == -1) blockIter = blockIter.GetWestNeighbor();

			//if next block is in unactivated chunk, return blank raycast
			if (blockIter.GetChunk() == nullptr)
			{
				return raycastResult;
			}

			//if next block is solid, hit
			if (blockIter.GetBlock()->IsSolid())
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = totalDistAtNextXCrossing;
				raycastResult.m_impactPos = startPosition + (directionNormal * totalDistAtNextXCrossing);
				if (stepDirectionX == -1)
				{
					raycastResult.m_impactNormal = Vec3(1.0f, 0.0f, 0.0f);
				}
				else
				{
					raycastResult.m_impactNormal = Vec3(-1.0f, 0.0f, 0.0f);
				}
				raycastResult.m_impactedBlock = blockIter;
				return raycastResult;
			}

			//otherwise, step forward
			totalDistAtNextXCrossing += distPerXCrossing;
		}
		else if (totalDistAtNextYCrossing <= totalDistAtNextXCrossing && totalDistAtNextYCrossing <= totalDistAtNextZCrossing)
		{
			//return blank raycast if we surpass max raycast distance
			if (totalDistAtNextYCrossing > distance)
			{
				return raycastResult;
			}

			if (stepDirectionY == 1) blockIter = blockIter.GetNorthNeighbor();
			else if (stepDirectionY == -1) blockIter = blockIter.GetSouthNeighbor();

			//if next block is in unactivated chunk, return blank raycast
			if (blockIter.GetChunk() == nullptr)
			{
				return raycastResult;
			}

			//if next block is solid, hit
			if (blockIter.GetBlock()->IsSolid())
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = totalDistAtNextYCrossing;
				raycastResult.m_impactPos = startPosition + (directionNormal * totalDistAtNextYCrossing);
				if (stepDirectionY == -1)
				{
					raycastResult.m_impactNormal = Vec3(0.0f, 1.0f, 0.0f);
				}
				else
				{
					raycastResult.m_impactNormal = Vec3(0.0f, -1.0f, 0.0f);
				}
				raycastResult.m_impactedBlock = blockIter;
				return raycastResult;
			}

			//otherwise, step forward
			totalDistAtNextYCrossing += distPerYCrossing;
		}
		else if (totalDistAtNextZCrossing <= totalDistAtNextYCrossing && totalDistAtNextZCrossing <= totalDistAtNextXCrossing)
		{
			//return blank raycast if we surpass max raycast distance
			if (totalDistAtNextZCrossing > distance)
			{
				return raycastResult;
			}

			if (stepDirectionZ == 1) blockIter = blockIter.GetSkywardNeighbor();
			else if (stepDirectionZ == -1) blockIter = blockIter.GetDownwardNeighbor();

			//if next block is in unactivated chunk, return blank raycast
			if (blockIter.GetChunk() == nullptr)
			{
				return raycastResult;
			}

			//if next block is solid, hit
			if (blockIter.GetBlock()->IsSolid())
			{
				raycastResult.m_didImpact = true;
				raycastResult.m_impactDist = totalDistAtNextZCrossing;
				raycastResult.m_impactPos = startPosition + (directionNormal * totalDistAtNextZCrossing);
				if (stepDirectionZ == -1)
				{
					raycastResult.m_impactNormal = Vec3(0.0f, 0.0f, 1.0f);
				}
				else
				{
					raycastResult.m_impactNormal = Vec3(0.0f, 0.0f, -1.0f);
				}
				raycastResult.m_impactedBlock = blockIter;
				return raycastResult;
			}

			//otherwise, step forward
			totalDistAtNextZCrossing += distPerZCrossing;
		}
	}

	//if you get here, return blank raycast
	return raycastResult;
}


//
//public lighting management functions
//
void World::ProcessDirtyLighting()
{
	while (m_dirtyBlocks.size() > 0)
	{
		ProcessNextDirtyLightBlock();
	}
}


//
//private lighting management functions
//
void World::ProcessNextDirtyLightBlock()
{
	//shouldn't ever reach this function with a queue size of zero, but just in case
	if (m_dirtyBlocks.size() == 0)
	{
		return;
	}
	
	//take block iterator off front of queue
	BlockIterator blockIter = m_dirtyBlocks.front();
	m_dirtyBlocks.pop_front();

	//if block iterator is somehow invalid, don't process it
	if (blockIter.GetChunk() == nullptr)
	{
		return;
	}

	//clear dirty light flag
	Block* block = blockIter.GetBlock();
	block->m_bitFlags = block->m_bitFlags & ~BLOCK_BIT_IS_LIGHT_DIRTY;

	//compute theoretically-correct light influence
	uint8_t correctOutdoorLightLevel = 0;
	uint8_t correctIndoorLightLevel = 0;
	uint8_t previousOutdoorLightLevel = block->GetOutdoorLightLevel();
	uint8_t previousIndoorLightLevel = block->GetIndoorLightLevel();
	
	//if sky, outdoor light value is always 15
	if ((block->m_bitFlags & BLOCK_BIT_IS_SKY) == BLOCK_BIT_IS_SKY)
	{
		correctOutdoorLightLevel = 15;
	}

	//if block emits indoor light, indoor light value is always /at least/ that value (be sure to add outdoor support if blocks can emit outdoor light without being sky)
	int blockDefID = block->m_blockType;
	BlockDefinition const* blockDef = BlockDefinition::GetBlockDefFromID(blockDefID);
	if (blockDef->m_lightEmissionValue > correctIndoorLightLevel)
	{
		correctIndoorLightLevel = static_cast<uint8_t>(blockDef->m_lightEmissionValue);
	}

	//if block is not opaque, check indoor and outdoor light levels of neighbors
	if (!blockDef->m_isOpaque)
	{
		BlockIterator eastNeighbor = blockIter.GetEastNeighbor();
		BlockIterator westNeighbor = blockIter.GetWestNeighbor();
		BlockIterator northNeighbor = blockIter.GetNorthNeighbor();
		BlockIterator southNeighbor = blockIter.GetSouthNeighbor();
		BlockIterator skywardNeighbor = blockIter.GetSkywardNeighbor();
		BlockIterator downwardNeighbor = blockIter.GetDownwardNeighbor();
		
		Block* eastBlock = nullptr;
		if (eastNeighbor.GetChunk() != nullptr) eastBlock = eastNeighbor.GetBlock();
		Block* westBlock = nullptr;
		if (westNeighbor.GetChunk() != nullptr) westBlock = westNeighbor.GetBlock();
		Block* northBlock = nullptr;
		if (northNeighbor.GetChunk() != nullptr) northBlock = northNeighbor.GetBlock();
		Block* southBlock = nullptr;
		if (southNeighbor.GetChunk() != nullptr) southBlock = southNeighbor.GetBlock();
		Block* skywardBlock = nullptr;
		if (skywardNeighbor.GetChunk() != nullptr) skywardBlock = skywardNeighbor.GetBlock();
		Block* downwardBlock = nullptr;
		if (downwardNeighbor.GetChunk() != nullptr) downwardBlock = downwardNeighbor.GetBlock();

		//compare to east indoor and outdoor
		if (eastBlock != nullptr)
		{
			uint8_t eastAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(eastBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t eastAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(eastBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (eastAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = eastAdjacentOutdoorLight;
			}
			if (eastAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = eastAdjacentIndoorLight;
			}
		}
		
		//compare to west indoor and outdoor
		if (westBlock != nullptr)
		{
			uint8_t westAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(westBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t westAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(westBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (westAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = westAdjacentOutdoorLight;
			}
			if (westAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = westAdjacentIndoorLight;
			}
		}

		//compare to north indoor and outdoor
		if (northBlock != nullptr)
		{
			uint8_t northAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(northBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t northAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(northBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (northAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = northAdjacentOutdoorLight;
			}
			if (northAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = northAdjacentIndoorLight;
			}
		}

		//compare to south indoor and outdoor
		if (southBlock != nullptr)
		{
			uint8_t southAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(southBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t southAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(southBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (southAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = southAdjacentOutdoorLight;
			}
			if (southAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = southAdjacentIndoorLight;
			}
		}

		//compare to skyward indoor and outdoor
		if (skywardBlock != nullptr)
		{
			uint8_t skywardAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(skywardBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t skywardAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(skywardBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (skywardAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = skywardAdjacentOutdoorLight;
			}
			if (skywardAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = skywardAdjacentIndoorLight;
			}
		}

		//compare to downward indoor and outdoor
		if (downwardBlock != nullptr)
		{
			uint8_t downwardAdjacentOutdoorLight = static_cast<uint8_t>(GetClamped(downwardBlock->GetOutdoorLightLevel(), 1, 15) - 1);
			uint8_t downwardAdjacentIndoorLight = static_cast<uint8_t>(GetClamped(downwardBlock->GetIndoorLightLevel(), 1, 15) - 1);
			if (downwardAdjacentOutdoorLight > correctOutdoorLightLevel)
			{
				correctOutdoorLightLevel = downwardAdjacentOutdoorLight;
			}
			if (downwardAdjacentIndoorLight > correctIndoorLightLevel)
			{
				correctIndoorLightLevel = downwardAdjacentIndoorLight;
			}
		}
	}

	//if either light influence value is incorrect...
	bool wasLightingUpdated = false;
	if (correctIndoorLightLevel != previousIndoorLightLevel)
	{
		wasLightingUpdated = true;

		//set block's indoor light level to new light level
		block->SetIndoorLightLevel(correctIndoorLightLevel);
	}
	if (correctOutdoorLightLevel != previousOutdoorLightLevel)
	{
		wasLightingUpdated = true;

		//set block's outdoor light level to new light level
		block->SetOutdoorLightLevel(correctOutdoorLightLevel);
	}

	if (wasLightingUpdated)
	{
		//mark chunk and chunks of neighboring blocks as having dirty meshes
		Chunk* chunk = blockIter.GetChunk();
		chunk->SetVertsAsDirty();
		BlockIterator eastNeighbor = blockIter.GetEastNeighbor();
		BlockIterator westNeighbor = blockIter.GetWestNeighbor();
		BlockIterator northNeighbor = blockIter.GetNorthNeighbor();
		BlockIterator southNeighbor = blockIter.GetSouthNeighbor();
		BlockIterator skywardNeighbor = blockIter.GetSkywardNeighbor();
		BlockIterator downwardNeighbor = blockIter.GetDownwardNeighbor();
		Chunk* eastNeighborChunk = eastNeighbor.GetChunk();
		Chunk* westNeighborChunk = westNeighbor.GetChunk();
		Chunk* northNeighborChunk = northNeighbor.GetChunk();
		Chunk* southNeighborChunk = southNeighbor.GetChunk();

		if (eastNeighborChunk != nullptr) eastNeighborChunk->m_areVertsDirty = true;	//fine that neighbor chunks could be same chunk because it'll just set the same bool
		if (westNeighborChunk != nullptr) westNeighborChunk->m_areVertsDirty = true;
		if (northNeighborChunk != nullptr) northNeighborChunk->m_areVertsDirty = true;
		if (southNeighborChunk != nullptr) southNeighborChunk->m_areVertsDirty = true;	//no need to check chunks of top and bottom blocks because they're always the same as ours

		//mark neighboring non-opaque blocks as having dirty lighting		
		if (eastNeighborChunk != nullptr && !eastNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(eastNeighbor.GetChunk(), eastNeighbor.GetBlockIndex());
		}
		if (westNeighborChunk != nullptr && !westNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(westNeighbor.GetChunk(), westNeighbor.GetBlockIndex());
		}
		if (northNeighborChunk != nullptr && !northNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(northNeighbor.GetChunk(), northNeighbor.GetBlockIndex());
		}
		if (southNeighborChunk != nullptr && !southNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(southNeighbor.GetChunk(), southNeighbor.GetBlockIndex());
		}
		if (skywardNeighbor.GetChunk() != nullptr && !skywardNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(skywardNeighbor.GetChunk(), skywardNeighbor.GetBlockIndex());
		}
		if (downwardNeighbor.GetChunk() != nullptr && !downwardNeighbor.GetBlock()->IsOpaque())
		{
			MarkLightingDirty(downwardNeighbor.GetChunk(), downwardNeighbor.GetBlockIndex());
		}
	}
}


void World::MarkLightingDirty(Chunk* chunk, int blockIndex)
{
	Block* block = &chunk->m_blocks[blockIndex];
	
	//if block isn't already in queue
	if ((block->m_bitFlags & BLOCK_BIT_IS_LIGHT_DIRTY) != BLOCK_BIT_IS_LIGHT_DIRTY)
	{
		//set block flag for dirty lighting
		block->m_bitFlags = block->m_bitFlags | BLOCK_BIT_IS_LIGHT_DIRTY;

		m_dirtyBlocks.emplace_back(BlockIterator(blockIndex, chunk));
	}
}


void World::UndirtyAllBlocksInChunk(Chunk* chunk)
{
	for (auto queueItr = m_dirtyBlocks.begin(); queueItr < m_dirtyBlocks.end(); queueItr++)
	{
		if (queueItr->GetChunk() == chunk)
		{
			m_dirtyBlocks.erase(queueItr);
		}
	}
}

