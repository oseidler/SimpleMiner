#include "Game/ChunkGenerateJob.hpp"


void ChunkGenerateJob::Execute()
{
	m_chunk->m_state = ChunkState::GENERATING;
	
	m_chunk->PopulateBlocks();

	m_chunk->m_state = ChunkState::COMPLETED;
}
