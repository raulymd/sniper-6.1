#include "dram_cntlr.h"
#include "memory_manager.h"
#include "core.h"
#include "log.h"
#include "subsecond_time.h"
#include "stats.h"
#include "fault_injection.h"
#include "magic_server.h"
#include <cstring>
#include <fstream>

//#include "Raul.h"
#define ENABLE_RAUL_MEMLOG

#define PAGE_SIZE 4096     //extracted by: $ getconf PAGE_SIZE

#if 0
   extern Lock iolock;
#  include "core_manager.h"
#  include "simulator.h"
#  define MYLOG(...) { ScopedLock l(iolock); fflush(stdout); printf("[%s] %d%cdr %-25s@%3u: ", itostr(getShmemPerfModel()->getElapsedTime()).c_str(), getMemoryManager()->getCore()->getId(), Sim()->getCoreManager()->amiUserThread() ? '^' : '_', __FUNCTION__, __LINE__); printf(__VA_ARGS__); printf("\n"); fflush(stdout); }
#else
#  define MYLOG(...) {}
#endif

//Raul codes
void Raul_dram_log(SubsecondTime now, IntPtr addr, char* RW, core_id_t requester)
{
   static bool log_file_opened(false);
   static std::ofstream Raul_ofs;
   if(log_file_opened == false)
   {
      log_file_opened = true;
      Raul_ofs.open ("dram.log", std::ofstream::out);
      Raul_ofs << "TIME \t\t\tADDR  \t\tTYPE \t\tCORE" << std::endl;
      //Raul_ofs << "TIME \t\t\tADDR  \t\t STADDR  \t\tTYPE \t\tCORE" << std::endl;
   }
   //if(Sim()->getMagicServer()->is_approx((UInt64)addr))
      Raul_ofs << now.getNS() << " \t\t" << addr  << " \t\t" << RW << " \t\t" << requester << std::endl;
      //Raul_ofs << now.getNS() << " \t\t" << addr << " \t\t" << ((UInt64)addr/PAGE_SIZE)*PAGE_SIZE << " \t\t" << RW << " \t\t" << requester << std::endl;
}

//Raul
std::set<UInt64> total_addrs;
std::set<UInt64> approx_addrs;


class TimeDistribution;

namespace PrL1PrL2DramDirectoryMSI
{

DramCntlr::DramCntlr(MemoryManagerBase* memory_manager,
      ShmemPerfModel* shmem_perf_model,
      UInt32 cache_block_size)
   : DramCntlrInterface(memory_manager, shmem_perf_model, cache_block_size)
   , m_reads(0)
   , m_writes(0)
   , overall_addr_cnter(0)
   , approx_addr_cnter(0)
{
   // std::cout << "inside dram controller" << std::endl;   -> only one dram controller
   m_dram_perf_model = DramPerfModel::createDramPerfModel(
         memory_manager->getCore()->getId(),
         cache_block_size);

   m_fault_injector = Sim()->getFaultinjectionManager()
      ? Sim()->getFaultinjectionManager()->getFaultInjector(memory_manager->getCore()->getId(), MemComponent::DRAM)
      : NULL;

   m_dram_access_count = new AccessCountMap[DramCntlrInterface::NUM_ACCESS_TYPES];
   registerStatsMetric("dram", memory_manager->getCore()->getId(), "reads", &m_reads);
   registerStatsMetric("dram", memory_manager->getCore()->getId(), "writes", &m_writes);
   //Raul
   registerStatsMetric("dram", memory_manager->getCore()->getId(), "overall_addr_cnter", &overall_addr_cnter);
   registerStatsMetric("dram", memory_manager->getCore()->getId(), "approx_addr_cnter", &approx_addr_cnter);
}

DramCntlr::~DramCntlr()
{
   printDramAccessCount();
   delete [] m_dram_access_count;

   delete m_dram_perf_model;

   //std::cout << "overall_addr_cnter: " << overall_addr_cnter << std::endl;
   //std::cout << "approx_addr_cnter: " << approx_addr_cnter << std::endl;
   //checking pages data structure
   // std::ofstream pageofs;
   // pageofs.open ("pages.log", std::ofstream::out);
   // pageofs << "staddr" << "\t\t" << "lastacc" << std::endl;
   // std::set<pagevar,pagecmp>::iterator it;
   // for(it=pages.begin(); it!=pages.end();++it)
   //    pageofs << it->staddr << "\t\t" << *(it->lastacc) << std::endl;
}

boost::tuple<SubsecondTime, HitWhere::where_t>
DramCntlr::getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf)
{
////////////////////////////////////////////////////////////////////////////////////////////////////   
#ifdef ENABLE_RAUL_MEMLOG
   // Raul_dram_log(now, address, (char*)"R", requester);
   //else
      //std::cout << "negative lifetime,"  << std::endl;
   //add_related_appvar(address);
   std::set<pagevar,pagecmp>::iterator it=get_pagevar(address);      //redundant
   if(it == pages.end())
      it = add_pagevar(address); 
   Sim()->getMagicServer()->update_liftime(address,now.getNS(),*(it->lastacc),it->related_approx_vars,true,getCacheBlockSize());
   if(now.getNS() > *(it->lastacc))     *(it->lastacc) = now.getNS();       //updating last page access
#endif

   if (Sim()->getFaultinjectionManager())
   {
      if (m_data_map.count(address) == 0)
      {
         m_data_map[address] = new Byte[getCacheBlockSize()];
         memset((void*) m_data_map[address], 0x00, getCacheBlockSize());
      }

      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into data_buf instead
      if (m_fault_injector)
         m_fault_injector->preRead(address, address, getCacheBlockSize(), (Byte*)m_data_map[address], now);

      memcpy((void*) data_buf, (void*) m_data_map[address], getCacheBlockSize());
   }

   SubsecondTime dram_access_latency = runDramPerfModel(requester, now, address, READ, perf);

   ++m_reads;
   {	//Raul
	UInt64 page_st_addr = (UInt64)address/PAGE_SIZE;
	std::pair<std::set<UInt64>::iterator,bool> ret;
	if(Sim()->getMagicServer()->is_block_approx((UInt64)address,getCacheBlockSize()))
	{
		ret = approx_addrs.insert(page_st_addr);
		if(ret.second)
			approx_addr_cnter++;
	}	   
	ret = total_addrs.insert(page_st_addr);
	if(ret.second)
		overall_addr_cnter++;
   }

   #ifdef ENABLE_DRAM_ACCESS_COUNT
   addToDramAccessCount(address, READ);
   #endif
   MYLOG("R @ %08lx latency %s", address, itostr(dram_access_latency).c_str());

   return boost::tuple<SubsecondTime, HitWhere::where_t>(dram_access_latency, HitWhere::DRAM);
}

boost::tuple<SubsecondTime, HitWhere::where_t>
DramCntlr::putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now)
{
   //std::cout << "Writing data to dram!" << std::endl;
#ifdef ENABLE_RAUL_MEMLOG
   // Raul_dram_log(now, address, (char*)"W", requester); 

   std::set<pagevar,pagecmp>::iterator it=get_pagevar(address);      //redundant
   if(it == pages.end())
      it = add_pagevar(address); 
   Sim()->getMagicServer()->update_liftime(address,now.getNS(),*(it->lastacc),it->related_approx_vars,false,getCacheBlockSize());
   if(now.getNS() > *(it->lastacc))     *(it->lastacc) = now.getNS();       //updating last page access
#endif

   if (Sim()->getFaultinjectionManager())
   {
      if (m_data_map[address] == NULL)
      {
         LOG_PRINT_ERROR("Data Buffer does not exist");
      }
      memcpy((void*) m_data_map[address], (void*) data_buf, getCacheBlockSize());

      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into data_buf instead
      if (m_fault_injector)
         m_fault_injector->postWrite(address, address, getCacheBlockSize(), (Byte*)m_data_map[address], now);
   }

   SubsecondTime dram_access_latency = runDramPerfModel(requester, now, address, WRITE, &m_dummy_shmem_perf);

   ++m_writes; 
   {	//Raul
	UInt64 page_st_addr = (UInt64)address/PAGE_SIZE;
	std::pair<std::set<UInt64>::iterator,bool> ret;
	if(Sim()->getMagicServer()->is_block_approx((UInt64)address,getCacheBlockSize()))
	{
		ret = approx_addrs.insert(page_st_addr);
		if(ret.second)
			approx_addr_cnter++;
	}	   
	ret = total_addrs.insert(page_st_addr);
	if(ret.second)
		overall_addr_cnter++;
   }
   
   #ifdef ENABLE_DRAM_ACCESS_COUNT
   addToDramAccessCount(address, WRITE);
   #endif
   MYLOG("W @ %08lx", address);

   return boost::tuple<SubsecondTime, HitWhere::where_t>(dram_access_latency, HitWhere::DRAM);
}

SubsecondTime
DramCntlr::runDramPerfModel(core_id_t requester, SubsecondTime time, IntPtr address, DramCntlrInterface::access_t access_type, ShmemPerf *perf)
{
   //static SubsecondTime  last_time;
   //static UInt64 count(0);
   //static UInt64 timesum(0);
   //UInt64 timeint = time.getNS()-last_time.getNS();
   //count++;
   //if(timeint < 5000)
//	   timesum += timeint;
   UInt64 pkt_size = getCacheBlockSize();
   SubsecondTime dram_access_latency = m_dram_perf_model->getAccessLatency(time, pkt_size, requester, address, access_type, perf);
   //std::cout  << "avg time between calls: " << timesum/count << std::endl;
   //last_time = time;
   return dram_access_latency;
}

void
DramCntlr::addToDramAccessCount(IntPtr address, DramCntlrInterface::access_t access_type)
{
   m_dram_access_count[access_type][address] = m_dram_access_count[access_type][address] + 1;
}

void
DramCntlr::printDramAccessCount()
{
   for (UInt32 k = 0; k < DramCntlrInterface::NUM_ACCESS_TYPES; k++)
   {
      for (AccessCountMap::iterator i = m_dram_access_count[k].begin(); i != m_dram_access_count[k].end(); i++)
      {
         if ((*i).second > 100)
         {
            LOG_PRINT("Dram Cntlr(%i), Address(0x%x), Access Count(%llu), Access Type(%s)",
                  m_memory_manager->getCore()->getId(), (*i).first, (*i).second,
                  (k == READ)? "READ" : "WRITE");
         }
      }
   }
}

bool DramCntlr::update_page_acc(UInt64 address,UInt64 now)
{
   pagevar tmp; tmp.staddr = (address/PAGE_SIZE)*PAGE_SIZE;
   std::set<pagevar,pagecmp>::iterator it=pages.find(tmp);  
   if(it == pages.end())   //page not present
   {
      tmp.lastacc = new UInt64(now);
      tmp.related_approx_vars = new std::set<UInt64>;
      pages.insert(tmp);   //making a new page
      return false;  
   }
   else
   {
      if(now > *(it->lastacc))
      {
         *(it->lastacc) = now;
         //std::cout << "now, lastacc  " << now << "\t" << *(it->lastacc) << std::endl;
      }
      return true;
   }
}

UInt64 DramCntlr::get_last_page_acc(UInt64 address)
{
   pagevar tmp; tmp.staddr = (address/PAGE_SIZE)*PAGE_SIZE;
   std::set<pagevar,pagecmp>::iterator it=pages.find(tmp); 
   if(it == pages.end())
      return 0;      //page not available
   else 
      return *(it->lastacc);
}

bool DramCntlr::add_related_appvar(UInt64 address)
{
   if(Sim()->getMagicServer()->is_block_approx(address,getCacheBlockSize())){
      pagevar tmp; tmp.staddr = (address/PAGE_SIZE)*PAGE_SIZE;
      std::set<pagevar,pagecmp>::iterator it=pages.find(tmp); 
      if(it == pages.end())
      {
         std::cout << "ERROR->page not available!" << std::endl;
         return true;      //if is approximate
      }
      else 
      {
         it->related_approx_vars->insert(address);
         return true;
      }
   }
   else
      return false;
}

std::set<pagevar,pagecmp>::iterator DramCntlr::get_pagevar(UInt64 address) 
{
   pagevar tmp; tmp.staddr = (address/PAGE_SIZE)*PAGE_SIZE;
   return pages.find(tmp);
}

std::set<pagevar,pagecmp>::iterator DramCntlr::add_pagevar(UInt64 address)
{
   pagevar tmp; tmp.staddr = (address/PAGE_SIZE)*PAGE_SIZE;
   tmp.lastacc = new UInt64(0);
   tmp.related_approx_vars = new std::set<UInt64>;
   pages.insert(tmp);   //making a new p   
   std::set<pagevar,pagecmp>::iterator it=pages.find(tmp);
   return it;
}

}


