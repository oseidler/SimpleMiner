#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB3.hpp"


//forward declarations
class Chunk;
class Block;


struct BlockIterator
{
//public member functions
public:
	//constructor
	explicit BlockIterator(int blockIndex, Chunk* chunk);

	//general accessors
	Block* GetBlock() const;
	Chunk* GetChunk() const;
	int	   GetBlockIndex() const;
	Vec3   GetWorldCenter() const;
	AABB3  GetBlockBounds() const;

	//neighbor accessors
	BlockIterator GetEastNeighbor() const;
	BlockIterator GetWestNeighbor() const;
	BlockIterator GetNorthNeighbor() const;
	BlockIterator GetSouthNeighbor() const;
	BlockIterator GetSkywardNeighbor() const;
	BlockIterator GetDownwardNeighbor() const;

//private member variables
private:
	int	   m_blockIndex = -1;
	Chunk* m_chunk = nullptr;
};
