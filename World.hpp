#pragma once
#include "Game/BlockIterator.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/JobSystem/JobSystem.hpp"
#include <deque>


//forward declarations
class Player;
class Game;
class Chunk;


//game version of raycast result struct
struct GameRaycastResult3D : public RaycastResult3D
{
	BlockIterator m_impactedBlock = BlockIterator(-1, nullptr);
};


//constants
constexpr float TIME_MIDNIGHT = 0.0f;
constexpr float TIME_DAWN = 0.25f;
constexpr float TIME_NOON = 0.5f;
constexpr float TIME_DUSK = 0.75f;


class World
{
//public member functions
public:
	//constructor and destructor
	World(Game* owner);
	~World();

	//game flow functions
	void Update();
	void Render() const;

	//chunk management functions
	bool FindNearestMissingChunk(IntVec2& out_chunkCoords, float activationRadius);
	void ActivateChunk(IntVec2 chunkCoords, Chunk* chunk);
	bool FindFarthestInactiveChunk(IntVec2& out_ChunkCoords, float deactivationRadius);
	void DeactivateChunk(IntVec2 chunkCoords);
	void DeactivateAllChunks();

	//raycast functions
	GameRaycastResult3D RaycastVsBlocks(Vec3 const& startPosition, Vec3 const& directionNormal, float distance);

	//lighting management functions
	void ProcessDirtyLighting();

//private member functions
private:
	void ProcessNextDirtyLightBlock();
	void MarkLightingDirty(Chunk* chunk, int blockIndex);
	void UndirtyAllBlocksInChunk(Chunk* chunk);

//public member variables
public:
	std::map<IntVec2, Chunk*> m_queuedChunks;
	std::map<IntVec2, Chunk*> m_activeChunks;

	Player* m_player = nullptr;
	Game*   m_game = nullptr;

	unsigned int m_worldSeed = 0;

	std::deque<BlockIterator> m_dirtyBlocks;

	float m_worldTime = 0.4f;
	float m_worldTimeScale = 200.0f;
	Rgba8 m_nightSkyColor = Rgba8(20, 20, 40);
	Rgba8 m_daySkyColor = Rgba8(200, 230, 255);
	Rgba8 m_currentSkyColor = m_nightSkyColor;
	Rgba8 m_nightOutdoorLightColor = Rgba8(50, 50, 160);
	Rgba8 m_dayOutdoorLightColor = Rgba8();
	Rgba8 m_currentOutdoorLightColor = m_nightSkyColor;
	Rgba8 m_baseIndoorLightColor = Rgba8(255, 230, 204);
	Rgba8 m_currentIndoorLightColor = m_baseIndoorLightColor;

	bool m_usingWorldShader = true;
	bool m_drawPlayerChunkBoundaries = false;

	float m_blockRaycastDistance = 8.0f;
	bool  m_lockRaycast = false;
	Vec3  m_blockRaycastStart = Vec3();
	Vec3  m_blockRaycastDirection = Vec3(1.0f, 0.0f, 0.0f);
};
