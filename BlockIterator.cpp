#include "Game/BlockIterator.hpp"
#include "Game/Block.hpp"
#include "Game/Chunk.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"


//
//constructor
//
BlockIterator::BlockIterator(int blockIndex, Chunk* chunk)
	: m_blockIndex(blockIndex)
	, m_chunk(chunk)
{
}


//
//general accessors
//
Block* BlockIterator::GetBlock() const
{
	if (m_chunk == nullptr)
	{
		return nullptr;
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	return &m_chunk->m_blocks[m_blockIndex];
}


Chunk* BlockIterator::GetChunk() const
{
	return m_chunk;
}


int BlockIterator::GetBlockIndex() const
{
	return m_blockIndex;
}


Vec3 BlockIterator::GetWorldCenter() const
{
	if (m_chunk == nullptr)
	{
		return Vec3();
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	int globalX = localX + (m_chunk->m_chunkCoords.x * CHUNK_SIZE_X);
	int globalY = localY + (m_chunk->m_chunkCoords.y * CHUNK_SIZE_Y);

	float globalXCenter = static_cast<float>(globalX) + 0.5f;
	float globalYCenter = static_cast<float>(globalY) + 0.5f;
	float globalZCenter = static_cast<float>(localZ) + 0.5f;

	return Vec3(globalXCenter, globalYCenter, globalZCenter);
}


AABB3 BlockIterator::GetBlockBounds() const
{
	if (m_chunk == nullptr)
	{
		return AABB3();
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");

	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	int globalX = localX + (m_chunk->m_chunkCoords.x * CHUNK_SIZE_X);
	int globalY = localY + (m_chunk->m_chunkCoords.y * CHUNK_SIZE_Y);

	float minX = static_cast<float>(globalX);
	float minY = static_cast<float>(globalY);
	float minZ = static_cast<float>(localZ);
	float maxX = minX + 1.0f;
	float maxY = minY + 1.0f;
	float maxZ = minZ + 1.0f;

	return AABB3(minX, minY, minZ, maxX, maxY, maxZ);
}


//
//neighbor accessors
//
BlockIterator BlockIterator::GetEastNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localX += 1;

	Chunk* chunk = m_chunk;

	if (localX == CHUNK_SIZE_X)
	{
		localX = 0;
		chunk = m_chunk->m_eastNeighbor;
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, chunk);
}


BlockIterator BlockIterator::GetWestNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localX -= 1;

	Chunk* chunk = m_chunk;

	if (localX == -1)
	{
		localX = 15;
		chunk = m_chunk->m_westNeighbor;
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, chunk);
}


BlockIterator BlockIterator::GetNorthNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localY += 1;

	Chunk* chunk = m_chunk;

	if (localY == CHUNK_SIZE_Y)
	{
		localY = 0;
		chunk = m_chunk->m_northNeighbor;
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, chunk);
}


BlockIterator BlockIterator::GetSouthNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localY -= 1;

	Chunk* chunk = m_chunk;

	if (localY == -1)
	{
		localY = 15;
		chunk = m_chunk->m_southNeighbor;
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, chunk);
}


BlockIterator BlockIterator::GetSkywardNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localZ += 1;

	if (localZ == CHUNK_SIZE_Z)
	{
		return BlockIterator(-1, nullptr);
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, m_chunk);
}


BlockIterator BlockIterator::GetDownwardNeighbor() const
{
	if (m_chunk == nullptr)
	{
		return BlockIterator(-1, nullptr);
	}
	GUARANTEE_OR_DIE(m_blockIndex >= 0 && m_blockIndex < CHUNK_TOTAL_BLOCKS, "Bad block index on block iterator!");
	
	// #ToDo: change to faster version
	int localX = m_blockIndex & (CHUNK_SIZE_X - 1);
	int localY = (m_blockIndex >> CHUNK_BITS_X) & (CHUNK_SIZE_Y - 1);
	int localZ = m_blockIndex >> (CHUNK_BITS_X + CHUNK_BITS_Y);

	localZ -= 1;

	if (localZ == -1)
	{
		return BlockIterator(-1, nullptr);
	}

	int blockIndex = localX + (localY << CHUNK_BITS_X) + (localZ << (CHUNK_BITS_X + CHUNK_BITS_Y));

	return BlockIterator(blockIndex, m_chunk);
}
