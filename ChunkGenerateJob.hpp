#pragma once
#include "Game/Chunk.hpp"
#include "Engine/JobSystem/Job.hpp"


class ChunkGenerateJob : public Job
{
//public member functions
public:
	ChunkGenerateJob(Chunk* chunk)
		: m_chunk(chunk)
	{}

	virtual void Execute() override;

//public member variables
public:
	Chunk* m_chunk = nullptr;
};
