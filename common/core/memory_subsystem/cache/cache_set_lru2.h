#ifndef CACHE_SET_LRU2_H
#define CACHE_SET_LRU2_H

#include "cache_set.h"

//by: RAUL

class CacheSetLRU2 : public CacheSet
{
   public:
      CacheSetLRU2(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      virtual ~CacheSetLRU2();

      virtual UInt32 getReplacementIndex(CacheCntlr *cntlr);
      void updateReplacementIndex(UInt32 accessed_index);

   protected:
      UInt8* m_lru_bits;
	  UInt32 approx_margin;	//RAUL
      //const UInt8 m_num_attempts;
      //CacheSetInfoLRU* m_set_info;
      //void moveToMRU(UInt32 accessed_index);
};

#endif /* CACHE_SET_LRU_H */
