#pragma once
#include "Game/Chunk.hpp"
#include "Game/Block.hpp"
#include "Game/BlockTemplate.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB3.hpp"


//chunk constants
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 7;

constexpr int CHUNK_SIZE_X = 1 << CHUNK_BITS_X;
constexpr int CHUNK_SIZE_Y = 1 << CHUNK_BITS_Y;
constexpr int CHUNK_SIZE_Z = 1 << CHUNK_BITS_Z;

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;

constexpr int CHUNK_LAYER_SIZE = CHUNK_SIZE_X * CHUNK_SIZE_Y;
constexpr int CHUNK_TOTAL_BLOCKS = CHUNK_LAYER_SIZE * CHUNK_SIZE_Z;


//generation constants
constexpr int BASE_TERRAIN_HEIGHT = 63;
constexpr int SEA_LEVEL = CHUNK_SIZE_Z / 2;
constexpr int OCEAN_FLOOR_DEPTH = 30;

constexpr float HUMIDITY_SAND_THRESHOLD = 0.45f;
constexpr float HUMIDITY_BEACH_THRESHOLD = 0.6f;
constexpr float HUMIDITY_MUSHROOM_THRESHOLD = 0.6f;
constexpr float TEMPERATURE_ICE_THRESHOLD = 0.5f;
constexpr float MAX_HILLINESS = 60.0f;
constexpr float MAX_OCEANNESS_THRESHOLD = 0.5f;
constexpr float LAVA_PIT_THRESHOLD = 0.75f;
constexpr float MUSHROOM_BASE_THRESHOLD = 0.5f;

constexpr float MAX_SAND_THICKNESS = 8.0f;
constexpr float MAX_ICE_THICKNESS = 10.0f;

constexpr float DIAMOND_RANGE_MAX = 0.001f;
constexpr float GOLD_RANGE_MAX = 0.005f + DIAMOND_RANGE_MAX;
constexpr float IRON_RANGE_MAX = 0.02f + GOLD_RANGE_MAX;
constexpr float COAL_RANGE_MAX = 0.05f + IRON_RANGE_MAX;

constexpr int CAVE_MIN_SEGMENTS = 10;
constexpr int CAVE_MAX_SEGMENTS = 20;
constexpr int CAVE_SEGMENT_MIN_LENGTH = 5;
constexpr int CAVE_SEGMENT_MAX_LENGTH = 10;
constexpr int CAVE_MIN_RADIUS = 2;
constexpr int CAVE_MAX_RADIUS = 6;
constexpr int CAVE_MAX_BLOCK_RADIUS = CAVE_MAX_SEGMENTS * CAVE_SEGMENT_MAX_LENGTH + CAVE_MAX_RADIUS;
constexpr int CAVE_MAX_CHUNK_RADIUS = CAVE_MAX_BLOCK_RADIUS / CHUNK_SIZE_X;	//incorrect for chunks that aren't same dimensions on x and y but oh well
constexpr int CAVE_ORIGIN_MIN_Z = 20;
constexpr int CAVE_ORIGIN_MAX_Z = BASE_TERRAIN_HEIGHT - 10;
constexpr float CAVE_MAX_ANGLE_CHANGE = 90.0f;
constexpr float CAVE_MAX_HEIGHT_CHANGE = 12.0f;
constexpr float CAVE_GENERATION_CHANCE = 0.025f;


//forward declarations
class VertexBuffer;
class World;


//generation state enum
enum class ChunkState
{
	QUEUED,
	GENERATING,
	COMPLETED,
	ACTIVATED
};


class Chunk
{
	friend class World;

//public member functions
public:
	//constructor and destructor
	Chunk(IntVec2 chunkCoords, World* world);
	~Chunk();

	//game flow functions
	void Update();
	void Render() const;

	//chunk utilities
	void PopulateBlocks();
	void AddCaves(unsigned int worldCaveSeed, std::vector<BlockTemplateEntry>& blockTemplateOrigins);
	void SetBlockType(int blockX, int blockY, int blockZ, std::string blockName);
	void SetBlockType(int blockIndex, std::string blockName);
	void SetBlockType(int blockIndex, uint8_t blockDefID);
	void SetBlockIsSky(int blockX, int blockY, int blockZ);
	std::string GetBlockType(int blockX, int blockY, int blockZ) const;
	std::string GetBlockType(int blockIndex) const;
	bool IsBlockOpaque(int blockX, int blockY, int blockZ) const;
	bool IsBlockOpaque(int blockIndex) const;
	int	 GetBlockLightEmissionValue(int blockIndex) const;
	bool SaveChunk();
	void LoadChunk();
	int  GetBlockIndexFromLocalCoords(int localX, int localY, int localZ) const;
	Vec3 GetChunkCenter() const;
	void SetVertsAsDirty();

//private member functions
private:
	//rendering functions
	void RebuildVertexes();
	void AddVertsForBlock(std::vector<Vertex_PCU>& verts, int blockIndex);

//public member variables
public:
	Block* m_blocks = nullptr;

	IntVec2 m_chunkCoords = IntVec2();
	AABB3	m_bounds = AABB3();

	World* m_world = nullptr;

	bool m_needsSaving = false;
	bool m_areVertsDirty = true;

	Chunk* m_eastNeighbor = nullptr;
	Chunk* m_westNeighbor = nullptr;
	Chunk* m_northNeighbor = nullptr;
	Chunk* m_southNeighbor = nullptr;

	std::atomic<ChunkState> m_state = ChunkState::QUEUED;

//private member variables
private:
	//rendering variables
	std::vector<Vertex_PCU> m_cpuMesh;
	VertexBuffer*			m_gpuMesh = nullptr;
};
