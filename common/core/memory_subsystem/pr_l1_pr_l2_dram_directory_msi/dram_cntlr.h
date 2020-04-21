#pragma once

// Define to re-enable DramAccessCount
//#define ENABLE_DRAM_ACCESS_COUNT

#include <unordered_map>

#include "dram_perf_model.h"
#include "shmem_msg.h"
#include "shmem_perf.h"
#include "fixed_types.h"
#include "memory_manager_base.h"
#include "dram_cntlr_interface.h"
#include "subsecond_time.h"
#include <set>

class FaultInjector;

//Raul codes
class pagevar
{
public:
   UInt64 staddr;
   UInt64* lastacc;
   std::set<UInt64>* related_approx_vars;
   pagevar():staddr(0){}
};

struct pagecmp {
  bool operator() (const pagevar& lhs, const pagevar& rhs) const
  {return lhs.staddr<rhs.staddr;}
};

namespace PrL1PrL2DramDirectoryMSI
{
   class DramCntlr : public DramCntlrInterface
   {
      private:
         std::unordered_map<IntPtr, Byte*> m_data_map;
         DramPerfModel* m_dram_perf_model;
         FaultInjector* m_fault_injector;
         //Raul
         std::set<pagevar,pagecmp> pages;

         typedef std::unordered_map<IntPtr,UInt64> AccessCountMap;
         AccessCountMap* m_dram_access_count;
         UInt64 m_reads, m_writes;
         
         ShmemPerf m_dummy_shmem_perf;

         SubsecondTime runDramPerfModel(core_id_t requester, SubsecondTime time, IntPtr address, DramCntlrInterface::access_t access_type, ShmemPerf *perf);

         void addToDramAccessCount(IntPtr address, access_t access_type);
         void printDramAccessCount(void);

      public:
         DramCntlr(MemoryManagerBase* memory_manager,
               ShmemPerfModel* shmem_perf_model,
               UInt32 cache_block_size);

         ~DramCntlr();

         //Raul
         bool update_page_acc(UInt64 address,UInt64 now);   //also includes page add
         std::set<pagevar,pagecmp>::iterator add_pagevar(UInt64 address);
         UInt64 get_last_page_acc(UInt64 address);
         bool add_related_appvar(UInt64 address);
         std::set<pagevar,pagecmp>::iterator get_pagevar(UInt64 address);
         UInt64 overall_addr_cnter;
         UInt64 approx_addr_cnter;


         DramPerfModel* getDramPerfModel() { return m_dram_perf_model; }

         // Run DRAM performance model. Pass in begin time, returns latency
         boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
         boost::tuple<SubsecondTime, HitWhere::where_t> putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);
   };
}
