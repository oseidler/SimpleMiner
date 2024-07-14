#include "Game/Block.hpp"
#include "Game/BlockDefinition.hpp"


//
//public accessors
//
uint8_t Block::GetOutdoorLightLevel() const
{
	//take top four bits of lighting var and return those
	uint8_t outdoorLighting = m_lighting >> 4;
	return outdoorLighting;
}


uint8_t Block::GetIndoorLightLevel() const
{
	//take bottom four bits of lighting var and return those
	uint8_t indoorLighting = m_lighting & 15;
	return indoorLighting;
}


bool Block::IsSky() const
{
	uint8_t maskedBit = m_bitFlags & BLOCK_BIT_IS_SKY;

	return maskedBit == BLOCK_BIT_IS_SKY;
}


bool Block::IsLightDirty() const
{
	uint8_t maskedBit = m_bitFlags & BLOCK_BIT_IS_LIGHT_DIRTY;

	return maskedBit == BLOCK_BIT_IS_LIGHT_DIRTY;
}


bool Block::IsOpaque() const
{
	BlockDefinition const* blockDef = BlockDefinition::GetBlockDefFromID(m_blockType);
	return blockDef->m_isOpaque;
}


bool Block::IsSolid() const
{
	BlockDefinition const* blockDef = BlockDefinition::GetBlockDefFromID(m_blockType);
	return blockDef->m_isSolid;
}


//
//public mutators
//
void Block::SetOutdoorLightLevel(uint8_t outdoorLighting)
{
	//clear the top four bits
	m_lighting = m_lighting & 15;

	//shift outdoor lighting to move the bottom four bits (where the actual value should be stored) to the top four
	outdoorLighting = outdoorLighting << 4;

	//put the two together;
	m_lighting += outdoorLighting;
}


void Block::SetIndoorLightLevel(uint8_t indoorLighting)
{
	//clear the bottom four bits
	m_lighting = m_lighting & 240;

	//put the two together
	m_lighting += indoorLighting;
}


void Block::SetIsSky(bool isSky)
{
	//clear the previous is sky bit
	m_bitFlags = m_bitFlags & (255 - BLOCK_BIT_IS_SKY);
	
	if (isSky)
	{
		//set the is sky bit to true
		m_bitFlags += BLOCK_BIT_IS_SKY;
	}
}


void Block::SetIsLightDirty(bool isLightDirty)
{
	//clear the previous is sky bit
	m_bitFlags = m_bitFlags & (255 - BLOCK_BIT_IS_LIGHT_DIRTY);

	if (isLightDirty)
	{
		//set the is sky bit to true
		m_bitFlags += BLOCK_BIT_IS_LIGHT_DIRTY;
	}
}
