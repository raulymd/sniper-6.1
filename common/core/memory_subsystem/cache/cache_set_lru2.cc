#include "cache_set_lru2.h"
#include "log.h"
#include "math.h"

// LRU2: the real LRU created by RAUL
// only works for approximate systems

CacheSetLRU2::CacheSetLRU2(
      CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize) :
   CacheSet(cache_type, associativity, blocksize)
{
   approx_margin = log2(m_associativity);	// 0 to approx_margin-1 cache ways are exact and others approximate
   m_lru_bits = new UInt8[m_associativity];
   for (UInt32 i = 0; i < approx_margin; i++)
      m_lru_bits[i] = i;	//exact
   for (UInt32 i = approx_margin; i < m_associativity; i++)
      m_lru_bits[i] = i-approx_margin;	//approx
}

CacheSetLRU2::~CacheSetLRU2() 
{
   delete [] m_lru_bits;
}

UInt32
CacheSetLRU2::getReplacementIndex(CacheCntlr *cntlr)
{
   // Invalidations may mess up the LRU bits
	if(m_approx_flag == false)		//exact address
	{
		for (UInt32 i = 0; i < approx_margin; i++)
		{
			if (!m_cache_block_info_array[i]->isValid())
			{
				updateReplacementIndex(i);
				return i;
			}
		}
		
		UInt32 index = 0;
		UInt8 max_bits = 0;
		for (UInt32 i = 0; i < approx_margin; i++)	//finding the maximum index
		{
			if (m_lru_bits[i] > max_bits && isValidReplacement(i))
			{
				index = i;
				max_bits = m_lru_bits[i];
			}
		}
		LOG_ASSERT_ERROR(index < approx_margin, "Error Finding LRU bits");
		updateReplacementIndex(index);
		return index;
	}
	else		//approximate address
	{
		for (UInt32 i = approx_margin; i < m_associativity; i++)
		{
			if (!m_cache_block_info_array[i]->isValid())
			{
				updateReplacementIndex(i);
				return i;
			}
		}
		
		UInt32 index = approx_margin;
		UInt8 max_bits = 0;
		for (UInt32 i = approx_margin; i < m_associativity; i++)	//finding the maximum index
		{
			if (m_lru_bits[i] > max_bits && isValidReplacement(i))
			{
				index = i;
				max_bits = m_lru_bits[i];
			}
		}
		LOG_ASSERT_ERROR(index < m_associativity, "Error Finding LRU bits");
		updateReplacementIndex(index);
		return index;		
	}

}

void
CacheSetLRU2::updateReplacementIndex(UInt32 accessed_index)
{
	if(accessed_index < approx_margin)
	{		//exact
		for (UInt32 i = 0; i < approx_margin; i++)
		{
			if (m_lru_bits[i] < m_lru_bits[accessed_index])
				m_lru_bits[i] ++;
		}
		m_lru_bits[accessed_index] = 0;
	}
	else
	{		//approx
		for (UInt32 i = approx_margin; i < m_associativity; i++)
		{
			if (m_lru_bits[i] < m_lru_bits[accessed_index])
				m_lru_bits[i] ++;
		}
		m_lru_bits[accessed_index] = 0;		
	}
}
