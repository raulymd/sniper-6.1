#ifndef MAGIC_SERVER_H
#define MAGIC_SERVER_H

#include "fixed_types.h"
#include "progress.h"
#include <string>
#include <set>
#include <vector>

//Raul codes

class appvar
{
public:
   UInt64 addr;
   UInt64* set_num; 
   // footprint values
   int* approx_add_cnt;
   int* approx_mul_cnt;
   int* L1_write_cnt;
   int* L1_read_cnt;
   
   int* L1_1_write_cnt;
   int* L1_1_read_cnt;
   int* L1_2_write_cnt;
   int* L1_2_read_cnt;
   int* L1_3_write_cnt;
   int* L1_3_read_cnt;
   
   int* L2_write_cnt;
   int* L2_read_cnt;
   UInt64* maxlifetime;
   // poten values
   int* poten_approx_add_cnt;
   int* poten_approx_mul_cnt;
   int* poten_L1_write_cnt;
   int* poten_L1_read_cnt;
   
   int* poten_L1_1_write_cnt;
   int* poten_L1_1_read_cnt;
   int* poten_L1_2_write_cnt;
   int* poten_L1_2_read_cnt;
   int* poten_L1_3_write_cnt;
   int* poten_L1_3_read_cnt;
   
   int* poten_L2_write_cnt;
   int* poten_L2_read_cnt;  
   UInt64* poten_maxlifetime;
   
   appvar();
   appvar(UInt64 addr);
   appvar(UInt64 addr, UInt64 set_num);
   std::string print(void) const;
   void upsave(void) const;     //because the iterator can not change values
   void reset(void) const;
   void inc_write_cnt( core_id_t coreid, std::string cachelvl) const;
   void inc_read_cnt( core_id_t coreid, std::string cachelvl) const;
   void inc_add_cnt(int) const;
   void inc_mul_cnt(int) const;
};

struct appcmp {
  bool operator() (const appvar& lhs, const appvar& rhs) const
  {return lhs.addr<rhs.addr;}
};

class MagicServer
{
   public:
      UInt64 approx_adds;   //Raul
      UInt64 approx_muls;   //Raul
      // data type to hold arguments in a HOOK_MAGIC_MARKER callback
      struct MagicMarkerType {
         thread_id_t thread_id;
         core_id_t core_id;
         UInt64 arg0, arg1;
         const char* str;
      };

      MagicServer();
      ~MagicServer();

      UInt64 Magic(thread_id_t thread_id, core_id_t core_id, UInt64 cmd, UInt64 arg0, UInt64 arg1);
      bool inROI(void) const { return m_performance_enabled; }
      static UInt64 getGlobalInstructionCount(void);

      // To be called while holding the thread manager lock
      UInt64 Magic_unlocked(thread_id_t thread_id, core_id_t core_id, UInt64 cmd, UInt64 arg0, UInt64 arg1);
      UInt64 setFrequency(UInt64 core_number, UInt64 freq_in_mhz);
      UInt64 getFrequency(UInt64 core_number);

      void enablePerformance();
      void disablePerformance();
      UInt64 setPerformance(bool enabled);
 

      UInt64 setInstrumentationMode(UInt64 sim_api_opt);

      void setProgress(float progress) { m_progress.setProgress(progress); }
  
      //Raul_ 
      //bool access_approx(UInt64 val,bool add);   //add or check val 
      bool 	add_approx(UInt64 addr, UInt64 set_num);
      bool 	del_approx(UInt64 addr);
      bool 	is_approx(UInt64 addr);
      bool 	is_block_approx(UInt64 addr, UInt32 block_size);
      int 	approx_addr_setnum(UInt64 addr);
      int 	approx_block_setnum(UInt64 addr, UInt32 block_size);	  
      void 	add_read_acc(UInt64 addr, core_id_t coreid, std::string cachelvl, UInt32 block_size);
      void 	add_write_acc(UInt64 addr, core_id_t coreid, std::string cachelvl, UInt32 block_size);
      bool	update_liftime(UInt64 addr,UInt64 now,UInt64 lpacc, std::set<UInt64>* rav,bool RW, UInt32 block_size);
	  void 	reset_cnters(UInt64,UInt32);
	  void 	upsave_cnters(UInt64,UInt32) ; 
	  void 	display_approx(void);
	  void  add_pure_acc(UInt64 addr,int corenum,std::string cachelvl,bool read );


   private:
      bool m_performance_enabled;
      Progress m_progress;
      //Raul_
      int myint;
      std::set<appvar,appcmp> approx_vars;   //set usedfor better search. log(n) complexity

};

#endif // SYNC_SERVER_H
