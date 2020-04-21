#include "magic_server.h"
#include "sim_api.h"
#include "simulator.h"
#include "thread_manager.h"
#include "logmem.h"
#include "performance_model.h"
#include "fastforward_performance_model.h"
#include "core_manager.h"
#include "dvfs_manager.h"
#include "hooks_manager.h"
#include "stats.h"
#include "timer.h"
#include "thread.h"
#include "config.hpp"
#include <iostream>
#include <set>

#define PAGE_SIZE 4096 
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define DEBUG 0

//approximate multipliers
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_000.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_001.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_002.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_003.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_004.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_005.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_006.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_007.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_008.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_009.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_010.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_011.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_012.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_013.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_014.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_015.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_016.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_017.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_018.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_019.c"
#include "/home/shahdaad/raul/utility/mul/mul16/mul16x16_020.c"

//Raul codes
UInt64 start_addr = -1;
UInt64 end_addr = 0;

int app_cnter = 0;

#define BIT_LENGTH 64

UInt64 rapcla(UInt64 a, UInt64 b, UInt64 w)		//w stands for window
{
	bool A[BIT_LENGTH-1];
	bool B[BIT_LENGTH-1];
	bool P[BIT_LENGTH-1];
	bool G[BIT_LENGTH-1];
	bool C[BIT_LENGTH-1] = {0};
	bool S[BIT_LENGTH-1];
	for(int i=0; i<BIT_LENGTH-1; i++)
	{
		UInt64 t = 1 << i;
		A[i] = ((t&a) == t);
		B[i] = ((t&b) == t);
		G[i] = A[i] & B[i];
		P[i] = A[i] ^ B[i];
	}
	
	for(int i=0; i<BIT_LENGTH-1; i++)  //i=2 c1
	{
		bool sum = 0;
		int st = i+1-w;
		if(st<0)	st=0;
		for(int j=st; j<i+1; j++)  //j=2
		{
			bool prod = 1;
			for (int z=j;z<i; z++) 
			{
				prod &= P[z];		//P1
			}
			if(j == 0)
				prod &= C[0];
			else
				prod &= G[j-1];
			sum |= prod;		//P0.P1.C0+G0.P1+G1
		}
		C[i] = sum;
		S[i] = A[i]^B[i]^C[i];
	}
	
	UInt64 s = 0;
	for(int i=0; i<BIT_LENGTH-1; i++)
	{
		if(S[i])
			s |= (1 << i);
	}
	return s;	
}

int appmul(short a, short b, int mulnum)
{
    if(a<0 && b>0)
        a = -1*a;
    if(a>0 && b<0)
        b = -1*b;
    int res = 0;
    switch(mulnum)
    {
        case  0:     res = mul16x16_000(a,b);    break; 
        case  1:     res = mul16x16_001(a,b);    break; 
        case  2:     res = mul16x16_002(a,b);    break; 
        case  3:     res = mul16x16_003(a,b);    break; 
        case  4:     res = mul16x16_004(a,b);    break; 
        case  5:     res = mul16x16_005(a,b);    break; 
        case  6:     res = mul16x16_006(a,b);    break; 
        case  7:     res = mul16x16_007(a,b);    break; 
        case  8:     res = mul16x16_008(a,b);    break; 
        case  9:     res = mul16x16_009(a,b);    break; 
        case 10:     res = mul16x16_010(a,b);    break; 
        case 11:     res = mul16x16_011(a,b);    break; 
        case 12:     res = mul16x16_012(a,b);    break; 
        case 13:     res = mul16x16_013(a,b);    break; 
        case 14:     res = mul16x16_014(a,b);    break; 
        case 15:     res = mul16x16_015(a,b);    break; 
        case 16:     res = mul16x16_016(a,b);    break; 
        case 17:     res = mul16x16_017(a,b);    break; 
        case 18:     res = mul16x16_018(a,b);    break; 
        case 19:     res = mul16x16_019(a,b);    break; 
        default:    LOG_PRINT_ERROR("ERROR: invalid approximate multiplier number (magic_server.cc)");
    }                     
    
    if(a*b < 0)
        return -1*res;
    else
        return res;
}


MagicServer::MagicServer()
      : m_performance_enabled(false)
{
   //std::cout << "inside MagicServer constructor!" << std::endl;
    registerStatsMetric("magic", 0, "approx_adds", &approx_adds);       //assumed one core for now
    registerStatsMetric("magic", 0, "approx_muls", &approx_muls);       //assumed one core for now
}

MagicServer::~MagicServer()
{ 
	//std::cout << "inside MagicServer desructor!" << std::endl;
}

UInt64 MagicServer::Magic(thread_id_t thread_id, core_id_t core_id, UInt64 cmd, UInt64 arg0, UInt64 arg1)
{
   ScopedLock sl(Sim()->getThreadManager()->getLock());

   return Magic_unlocked(thread_id, core_id, cmd, arg0, arg1);
}

//Raul_
// bool MagicServer::access_approx(UInt64 val,bool add)
// {
//    //adding Raul_codes
//    static std::set<UInt64> approx_vars;   //set usedfor better search. log(n) complexity
//    std::set<UInt64>::iterator it;
//    if(add)
//    {
//       approx_vars.insert(val);
//       std::cout << "Approx var added: " << val << std::endl;
//       return true;
//    }
//    else
//    {
//       it = approx_vars.find(val);
//       if(it == approx_vars.end())
//          return false;
//       else
//          return true;
//    }
// }

///////////////////////////////////////////////

appvar::appvar()		//appvar constructor
{
	//pointer initilization does not work -> simulator hangs
	/*
   addr = 0;
   set_num = new UInt64(0);
   //footprint values
   L1_write_cnt = new int(0) ;
   L1_read_cnt = new int(0);
   L1_1_write_cnt = new int(0);
   L1_1_read_cnt = new int(0);   
   L1_2_write_cnt = new int(0);
   L1_2_read_cnt = new int(0);   
   L1_3_write_cnt = new int(0);
   L1_3_read_cnt = new int(0);
   L2_write_cnt = new int(0);
   L2_read_cnt = new int(0);
   maxlifetime = new UInt64(0);
   approx_add_cnt = new int(0);
   approx_mul_cnt = new int(0);
	//poten values
   poten_L1_write_cnt = new int(0) ;
   poten_L1_read_cnt = new int(0);
   poten_L1_1_write_cnt = new int(0);
   poten_L1_1_read_cnt = new int(0);  
   poten_L1_2_write_cnt = new int(0);
   poten_L1_2_read_cnt = new int(0);   
   poten_L1_3_write_cnt = new int(0);
   poten_L1_3_read_cnt = new int(0);
   poten_L2_write_cnt = new int(0);
   poten_L2_read_cnt = new int(0);
   poten_maxlifetime = new UInt64(0);	
   poten_approx_add_cnt = new int(0);
   poten_approx_mul_cnt = new int(0);
   */
}

//// appvar
appvar::appvar(UInt64 adr):appvar()
{
	//addr = adr;
}

appvar::appvar(UInt64 adr, UInt64 sn):appvar()
{
	//addr = adr;
	//*set_num = sn;
}

void appvar::upsave()   const
{
	//if(addr == 6338562)
	//{
	//	std::cout << "[MAGIC] inside upsave_cnters for: " << addr << " value: " << *approx_add_cnt << " and poten: " << *(poten_approx_add_cnt) << std::endl;
	//	std::cout << "[MAGIC] max value is : " << MAX(*(poten_approx_add_cnt)   ,*(approx_add_cnt)) << std::endl;
	//}
    //int pre_cnt = *(L1_write_cnt) + *(L2_write_cnt) + *(L1_read_cnt) + *(L2_read_cnt);
	//int cur_cnt = *(poten_L1_write_cnt) + *(poten_L2_write_cnt) + *(poten_L1_read_cnt) + *(poten_L2_read_cnt);
	*(L1_write_cnt)	    = MAX(*(poten_L1_write_cnt)	    ,*(L1_write_cnt));
	*(L1_read_cnt)	    = MAX(*(poten_L1_read_cnt)	    ,*(L1_read_cnt));
	*(L1_1_write_cnt)	= MAX(*(poten_L1_1_write_cnt)	,*(L1_1_write_cnt));
	*(L1_1_read_cnt)	= MAX(*(poten_L1_1_read_cnt)	,*(L1_1_read_cnt)	);
	*(L1_2_write_cnt)	= MAX(*(poten_L1_2_write_cnt)	,*(L1_2_write_cnt));
	*(L1_2_read_cnt)	= MAX(*(poten_L1_2_read_cnt)	,*(L1_2_read_cnt)	);
	*(L1_3_write_cnt)	= MAX(*(poten_L1_3_write_cnt)	,*(L1_3_write_cnt));
	*(L1_3_read_cnt)	= MAX(*(poten_L1_3_read_cnt)	,*(L1_3_read_cnt)	);			
	*(L2_write_cnt)	    = MAX(*(poten_L2_write_cnt)	    ,*(L2_write_cnt));
	*(L2_read_cnt)	    = MAX(*(poten_L2_read_cnt)	    ,*(L2_read_cnt)); 	   
    *(approx_add_cnt)   = MAX(*(poten_approx_add_cnt)   ,*(approx_add_cnt));
    *(approx_mul_cnt)   = MAX(*(poten_approx_mul_cnt)   ,*(approx_mul_cnt));
	
	//if(addr == 6338562)
	//std::cout << "[MAGIC] gotted value is : " << *(approx_add_cnt) << std::endl;
}

void appvar::reset()    const
{
	
    //std::cout << " inside reset_cnters" << std::endl;
	*(poten_L1_write_cnt)	= 0;
	*(poten_L1_read_cnt)	= 0;
	*(poten_L1_1_write_cnt)	= 0;
	*(poten_L1_1_read_cnt)	= 0;
	*(poten_L1_2_write_cnt)	= 0;
	*(poten_L1_2_read_cnt)	= 0;
	*(poten_L1_3_write_cnt)	= 0;
	*(poten_L1_3_read_cnt)	= 0;
	*(poten_L2_write_cnt)	= 0;
	*(poten_L2_read_cnt)	= 0;   
    //*(poten_approx_add_cnt) = 0;        //add_cnt is directly added by source code
    //*(poten_approx_mul_cnt) = 0;        //mul_cnt is directly added by source code
	
}


std::string appvar::print(void)  const
{
	
	std::ostringstream oss;
	oss << addr << " \t\t" 
		<< *L1_read_cnt << " \t\t"
		<< *L1_write_cnt << " \t\t"
		/*
		<< *L1_1_read_cnt << " \t\t"
		<< *L1_1_write_cnt << " \t\t"
		<< *L1_2_read_cnt << " \t\t"
		<< *L1_2_write_cnt << " \t\t"
		<< *L1_3_read_cnt << " \t\t"
		<< *L1_3_write_cnt << " \t\t"	
		*/		
		<< *L2_read_cnt << " \t\t" 
		<< *L2_write_cnt << " \t\t"
		<< *maxlifetime << " \t\t"
		<< *approx_add_cnt << " \t\t"
		<< *approx_mul_cnt
		;
	return oss.str();
	
	//return "HI";
}

void appvar::inc_write_cnt( core_id_t cid, std::string cachelvl) const
{
	
	if(cachelvl == "L1-D" || cachelvl == "l1d")
    {   
        if(cid == 0)
            *(poten_L1_write_cnt) = *(poten_L1_write_cnt)+1;
        else if(cid == 1)
            *(poten_L1_1_write_cnt) = *(poten_L1_1_write_cnt)+1;
        else if(cid == 2)
            *(poten_L1_2_write_cnt) = *(poten_L1_2_write_cnt)+1;
        else if(cid == 3)
            *(poten_L1_3_write_cnt) = *(poten_L1_3_write_cnt)+1;			
        else
            std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;                
    }
	if(cachelvl == "L2" || cachelvl == "l2" )
        *(poten_L2_write_cnt) = *(poten_L2_write_cnt)+1;   

}

void appvar::inc_read_cnt( core_id_t cid, std::string cachelvl) const
{
	
    if(cachelvl == "L1-D" || cachelvl == "l1d")
    {
		if(cid == 0)
            *(poten_L1_read_cnt) = *(poten_L1_read_cnt)+1;
        else if(cid == 1)
            *(poten_L1_1_read_cnt) = *(poten_L1_1_read_cnt)+1;
        else if(cid == 2)
            *(poten_L1_2_read_cnt) = *(poten_L1_2_read_cnt)+1;
        else if(cid == 3)
            *(poten_L1_3_read_cnt) = *(poten_L1_3_read_cnt)+1;			
        else
            std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;
    }
	if(cachelvl == "L2" || cachelvl == "l2")
		*(poten_L2_read_cnt) = *(poten_L2_read_cnt)+1;   
	
}

void appvar::inc_add_cnt(int val) const
{
    *(poten_approx_add_cnt) = *(poten_approx_add_cnt)+val;	
}

void appvar::inc_mul_cnt(int val) const
{
    *(poten_approx_mul_cnt) = *(poten_approx_mul_cnt)+val;	
}

///////////////////////////////////////////////

UInt64 MagicServer::Magic_unlocked(thread_id_t thread_id, core_id_t core_id, UInt64 cmd, UInt64 arg0, UInt64 arg1)
{
   switch(cmd)
   {
      case SIM_CMD_ROI_TOGGLE:
         if (Sim()->getConfig()->getSimulationROI() == Config::ROI_MAGIC)
         {
            return setPerformance(! m_performance_enabled);
         }
         else
         {
            return 0;
         }
      case SIM_CMD_ROI_START:
        std::cout << "[MAGIC] ROI begin" << std::endl;
         Sim()->getHooksManager()->callHooks(HookType::HOOK_APPLICATION_ROI_BEGIN, 0);
         if (Sim()->getConfig()->getSimulationROI() == Config::ROI_MAGIC)
         {
            return setPerformance(true);
         }
         else
         {
            return 0;
         }
      case SIM_CMD_ROI_END:
        std::cout << "[MAGIC] ROI end" << std::endl;
         Sim()->getHooksManager()->callHooks(HookType::HOOK_APPLICATION_ROI_END, 0);
         if (Sim()->getConfig()->getSimulationROI() == Config::ROI_MAGIC)
         {
            return setPerformance(false); 
         }
         else
         {
            return 0;
         }
      case SIM_CMD_MHZ_SET:
         return setFrequency(arg0, arg1);
      case SIM_CMD_NAMED_MARKER:
      {
         char str[256];
         Core *core = Sim()->getCoreManager()->getCoreFromID(core_id);
         core->accessMemory(Core::NONE, Core::READ, arg1, str, 256, Core::MEM_MODELED_NONE);
         str[255] = '\0';

         MagicMarkerType args = { thread_id: thread_id, core_id: core_id, arg0: arg0, arg1: 0, str: str };
         Sim()->getHooksManager()->callHooks(HookType::HOOK_MAGIC_MARKER, (UInt64)&args);
         return 0;
      }
      case SIM_CMD_SET_THREAD_NAME:
      {
         char str[256];
         Core *core = Sim()->getCoreManager()->getCoreFromID(core_id);
         core->accessMemory(Core::NONE, Core::READ, arg0, str, 256, Core::MEM_MODELED_NONE);
         str[255] = '\0';

         Sim()->getStatsManager()->logEvent(StatsManager::EVENT_THREAD_NAME, SubsecondTime::MaxTime(), core_id, thread_id, 0, 0, str);
         Sim()->getThreadManager()->getThreadFromID(thread_id)->setName(str);
         return 0;
      }
      case SIM_CMD_MARKER: 
      {
         //adding Raul_codes
         //static int cnter(0);
         if(arg0%100 == 07)
         {
			 
			//if(arg0/100 == 1)	 				//0: src addresses for sobel
				//add_approx(arg1, 0); 		//1: dst addresses for sobel
				add_approx(arg1, arg0/100); 		
		

            //cnter++;
            //if(add_approx(arg1,arg0/100) && arg0/100==0)
            //    std::cout << cnter << " Approximate address added: " << (UInt64)arg1  << ", set number: " << arg0/100 << std::endl;  
            // PerformanceModel *perf = Sim()->getCoreManager()->getCoreFromID(0)->getPerformanceModel();
            // std::cout << "current time: " << perf->getNonIdleElapsedTime().getNS() << "\t" << perf->getFastforwardPerformanceModel()->getFastforwardedTime().getNS() << std::endl;
			//std::cout << cnter << std::endl;		// number of calls is correct
         }
         if(arg0%100 == 06)
         {
            // pring all variable log when first entered here
            static bool log_file_opened(false);
            static std::ofstream ofs;
			String s_injector = Sim()->getCfg()->getString("fault_injection/injector");
            if(log_file_opened == false && s_injector == "none")		//performing memory log (if random -> fault injecting)
            {
				//std::cout << "start_addr: " << start_addr << " end_addr: " << end_addr << std::endl;
				//display_approx();
				//std::cout << "approx size: " << approx_vars.size() << std::endl;
               ofs.open("approx.log");
               if(Sim()->getCfg()->getBoolDefault("mem_footprint/enable",false))
               {
                   //ofs << "ADDR \t\tCORE \t\tLEVEL \t\tREADS \t\tWRITES \t\tLIFETIME" << std::endl;
                   ofs << "ADDR  \t\tL1_READS(0) \t\tL1_WRITES(0) \t\tL1_READS(1) \t\tL1_WRITES(1) \t\tL1_READS(2) \t\tL1_WRITES(2) \t\tL1_READS(3) \t\tL1_WRITES(3) \t\tL2_READS \t\tL2_WRITES \t\tLIFETIME" << std::endl;		//type 2
                   log_file_opened = true;
                   std::set<appvar,appcmp>::iterator it;
                   for(it=approx_vars.begin(); it != approx_vars.end(); ++it)
                   {	
                        it->upsave();       //some approx entries are not upsaved even till the end of execution.
						ofs << it->print() << std::endl;
                        //if(it->addr % 64 == 0)
							/*
                         ofs << it->addr << " \t\t" 
                            << *(it->L1_read_cnt) << " \t\t"
                            << *(it->L1_write_cnt) << " \t\t"
                            << *(it->L1_1_read_cnt) << " \t\t"
                            << *(it->L1_1_write_cnt) << " \t\t"
                            << *(it->L1_2_read_cnt) << " \t\t"
                            << *(it->L1_2_write_cnt) << " \t\t"
                            << *(it->L1_3_read_cnt) << " \t\t"
                            << *(it->L1_3_write_cnt) << " \t\t"							
                            << *(it->L2_read_cnt) << " \t\t" 
                            << *(it->L2_write_cnt) << " \t\t"
                            << *(it->maxlifetime) <<   std::endl;	
							*/							
                    }	
               }
               else
               {
                    ofs << "mem footprint disabled!" << std::endl;
               }
               ofs.close();
            }            
            del_approx(arg1);
         }
         //end

         MagicMarkerType args = { thread_id: thread_id, core_id: core_id, arg0: arg0, arg1: arg1, str: NULL };
         Sim()->getHooksManager()->callHooks(HookType::HOOK_MAGIC_MARKER, (UInt64)&args);
         return 0;
      }
      case SIM_CMD_USER:
      {
		 
         MagicMarkerType args = { thread_id: thread_id, core_id: core_id, arg0: arg0, arg1: arg1, str: NULL };
         return Sim()->getHooksManager()->callHooks(HookType::HOOK_MAGIC_USER, (UInt64)&args, true /* expect return value */);
      }
      case SIM_CMD_INSTRUMENT_MODE:
         return setInstrumentationMode(arg0);
      case SIM_CMD_MHZ_GET:
         return getFrequency(arg0);
	  case SIM_CMD_ADD_APPROX_ADDRS:
	  {
			for (unsigned int i=0; i<arg1; i++)
            {
                bool ret = add_approx(arg0+i,0);
                if(ret == false)
                    std::cout << "[MAGIC] warning adding new approx address, address already exists." << std::endl;
            }
            std::cout << "[MAGIC] "<< app_cnter << " approx addrs added, start addr: " << arg0 << " size: " << arg1 << std::endl;
			return 0;
	  }
	  case SIM_CMD_DEL_APPROX_ADDRS:
	  {
            static bool log_file_opened(false);
            static std::ofstream ofs;
			String s_injector = Sim()->getCfg()->getString("fault_injection/injector");
            if(log_file_opened == false && s_injector == "none")		//performing memory log (if random -> fault injecting)
            {
				//std::cout << "start_addr: " << start_addr << " end_addr: " << end_addr << std::endl;
				//display_approx();
				//std::cout << "approx size: " << approx_vars.size() << std::endl;
               ofs.open("approx.log");
               if(Sim()->getCfg()->getBoolDefault("mem_footprint/enable",false))
               {
                   //ofs << "ADDR \t\tCORE \t\tLEVEL \t\tREADS \t\tWRITES \t\tLIFETIME" << std::endl;
                   ofs << "ADDR  \t\tL1_READS \t\tL1_WRITES \t\tL2_READS \t\tL2_WRITES \t\tLIFETIME \t\tAPPROX_ADD \t\tAPPROX_MUL" << std::endl;		//type 2
                   log_file_opened = true;
                   std::set<appvar,appcmp>::iterator it;
                   for(it=approx_vars.begin(); it != approx_vars.end(); ++it)
                   {		
                        //if(it->addr % 64 == 0)
						ofs << it->print() << std::endl;
                    }	
               }
               else
               {
                    ofs << "mem footprint disabled!" << std::endl;
               }
               ofs.close();
            } 
			
			//deleting variables
			for (unsigned int i=0; i<arg1; i++)
				del_approx(arg0+i);
			return 0;
	  }
	  case SIM_CMD_ADD_OP:
	  {
            approx_adds++;
			if(Sim()->getCfg()->getString("fault_injection/injector") == "random")
			{
				return (UInt64)rapcla(arg0,arg1,16);        //using rapcla as approximate adder
			}
			else
				return arg0+arg1;
	  }
	  case SIM_CMD_ADD_CNT:
	  {
			appvar tmp;		tmp.addr = arg0; 		//approx address
			std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
			if(it != approx_vars.end())		//found
			{
				//OK
				//if(arg0 == 6338562)
				//std::cout << "[MAGIC] adding ADD counter, address: " << arg0 << " value: " << arg1 << std::endl; 
				it->inc_add_cnt(arg1);
				return 1;
			}
            else 
            {
                std::cout << "[MAGIC] warning, approx addr not found to inc_add_cnt" << std::endl;
                return 0;
            }
	  }
      case SIM_CMD_MUL_OP:
      {
            approx_muls++;
			if(Sim()->getCfg()->getString("fault_injection/injector") == "random")
			{
				return appmul((short)arg0,(short)arg1,19);//(UInt64)appmul(arg0,arg1);        //using rapcla as approximate adder
			}
			else
            {
                return arg0*arg1;           
            }
      }
      case SIM_CMD_MUL_CNT:
      {
			appvar tmp;		tmp.addr = arg0; 		//approx address
			std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
			if(it != approx_vars.end())		//found
			{
				//OK
				//if(arg0 == 6338562)
				//std::cout << "[MAGIC] adding ADD counter, address: " << arg0 << " value: " << arg1 << std::endl; 
				it->inc_mul_cnt(arg1);
				return 1;
			}
            else
            {
                std::cout << "[MAGIC] warning, approx addr not found to inc_mul_cnt" << std::endl;
                return 0;
            }
      }
	  
      default:
         LOG_ASSERT_ERROR(false, "Got invalid Magic %lu, arg0(%lu) arg1(%lu)", cmd, arg0, arg1);
   }
   return 0;
}

UInt64 MagicServer::getGlobalInstructionCount(void)
{
   UInt64 ninstrs = 0;
   for (UInt32 i = 0; i < Sim()->getConfig()->getApplicationCores(); i++)
      ninstrs += Sim()->getCoreManager()->getCoreFromID(i)->getInstructionCount();
   return ninstrs;
}

static Timer t_start;
UInt64 ninstrs_start;
__attribute__((weak)) void PinDetach(void) {}

void MagicServer::enablePerformance()
{
   Sim()->getStatsManager()->recordStats("roi-begin");
   ninstrs_start = getGlobalInstructionCount();
   t_start.start();

   Simulator::enablePerformanceModels();
   Sim()->setInstrumentationMode(InstMode::inst_mode_roi, true /* update_barrier */);
}

void MagicServer::disablePerformance()
{
   Simulator::disablePerformanceModels();
   Sim()->getStatsManager()->recordStats("roi-end");

   float seconds = t_start.getTime() / 1e9;
   UInt64 ninstrs = getGlobalInstructionCount() - ninstrs_start;
   UInt64 cycles = SubsecondTime::divideRounded(Sim()->getClockSkewMinimizationServer()->getGlobalTime(),
                                                Sim()->getCoreManager()->getCoreFromID(0)->getDvfsDomain()->getPeriod());
   printf("[SNIPER] Simulated %.1fM instructions, %.1fM cycles, %.2f IPC\n",
      ninstrs / 1e6,
      cycles / 1e6,
      float(ninstrs) / (cycles ? cycles : 1));
   printf("[SNIPER] Simulation speed %.1f KIPS (%.1f KIPS / target core - %.1fns/instr)\n",
      ninstrs / seconds / 1e3,
      ninstrs / seconds / 1e3 / Sim()->getConfig()->getApplicationCores(),
      seconds * 1e9 / (float(ninstrs ? ninstrs : 1.) / Sim()->getConfig()->getApplicationCores()));

   PerformanceModel *perf = Sim()->getCoreManager()->getCoreFromID(0)->getPerformanceModel();
   if (perf->getFastforwardPerformanceModel()->getFastforwardedTime() > SubsecondTime::Zero())
   {
      // NOTE: Prints out the non-idle ratio for core 0 only, but it's just indicative anyway
      double ff_ratio = double(perf->getFastforwardPerformanceModel()->getFastforwardedTime().getNS())
                      / double(perf->getNonIdleElapsedTime().getNS());
      double percent_detailed = 100. * (1. - ff_ratio);
      printf("[SNIPER] Sampling: executed %.2f%% of simulated time in detailed mode\n", percent_detailed);
   }

   fflush(NULL);

   Sim()->setInstrumentationMode(InstMode::inst_mode_end, true /* update_barrier */);
   PinDetach();
}

void print_allocations();

UInt64 MagicServer::setPerformance(bool enabled)
{
   if (m_performance_enabled == enabled)
      return 1;

   m_performance_enabled = enabled;

   //static bool enabled = false;
   static Timer t_start;
   //ScopedLock sl(l_alloc);

   if (m_performance_enabled)
   {
      printf("[SNIPER] Enabling performance models\n");
      fflush(NULL);
      t_start.start();
      logmem_enable(true);
      Sim()->getHooksManager()->callHooks(HookType::HOOK_ROI_BEGIN, 0);
   }
   else
   {
      Sim()->getHooksManager()->callHooks(HookType::HOOK_ROI_END, 0);
      printf("[SNIPER] Disabling performance models\n");
      float seconds = t_start.getTime() / 1e9;
      printf("[SNIPER] Leaving ROI after %.2f seconds\n", seconds);
      fflush(NULL);
      logmem_enable(false);
      logmem_write_allocations();
   }

   if (enabled)
      enablePerformance();
   else
      disablePerformance();

   return 0;
}

UInt64 MagicServer::setFrequency(UInt64 core_number, UInt64 freq_in_mhz)
{
   UInt32 num_cores = Sim()->getConfig()->getApplicationCores();
   UInt64 freq_in_hz;
   if (core_number >= num_cores)
      return 1;
   freq_in_hz = 1000000 * freq_in_mhz;

   printf("[SNIPER] Setting frequency for core %" PRId64 " in DVFS domain %d to %" PRId64 " MHz\n", core_number, Sim()->getDvfsManager()->getCoreDomainId(core_number), freq_in_mhz);

   if (freq_in_hz > 0)
      Sim()->getDvfsManager()->setCoreDomain(core_number, ComponentPeriod::fromFreqHz(freq_in_hz));
   else {
      Sim()->getThreadManager()->stallThread_async(core_number, ThreadManager::STALL_BROKEN, SubsecondTime::MaxTime());
      Sim()->getCoreManager()->getCoreFromID(core_number)->setState(Core::BROKEN);
   }

   // First set frequency, then call hooks so hook script can find the new frequency by querying the DVFS manager
   Sim()->getHooksManager()->callHooks(HookType::HOOK_CPUFREQ_CHANGE, core_number);

   return 0;
}

UInt64 MagicServer::getFrequency(UInt64 core_number)
{
   UInt32 num_cores = Sim()->getConfig()->getApplicationCores();
   if (core_number >= num_cores)
      return UINT64_MAX;

   const ComponentPeriod *per = Sim()->getDvfsManager()->getCoreDomain(core_number);
   return per->getPeriodInFreqMHz();
}

UInt64 MagicServer::setInstrumentationMode(UInt64 sim_api_opt)
{
   InstMode::inst_mode_t inst_mode;
   switch (sim_api_opt)
   {
   case SIM_OPT_INSTRUMENT_DETAILED:
      inst_mode = InstMode::DETAILED;
      break;
   case SIM_OPT_INSTRUMENT_WARMUP:
      inst_mode = InstMode::CACHE_ONLY;
      break;
   case SIM_OPT_INSTRUMENT_FASTFORWARD:
      inst_mode = InstMode::FAST_FORWARD;
      break;
   default:
      LOG_PRINT_ERROR("Unexpected magic instrument opt type: %lx.", sim_api_opt);
   }
   Sim()->setInstrumentationMode(inst_mode, true /* update_barrier */);

   return 0;
}



bool MagicServer::add_approx(UInt64 address, UInt64 set_number)		//expects byte address
{
   appvar tmp; 
   
   tmp.addr = address;
   tmp.set_num = new UInt64(0);
   //footprint values
   tmp.L1_write_cnt = new int(0) ;
   tmp.L1_read_cnt = new int(0);
   tmp.L1_1_write_cnt = new int(0);
   tmp.L1_1_read_cnt = new int(0);   
   tmp.L1_2_write_cnt = new int(0);
   tmp.L1_2_read_cnt = new int(0);   
   tmp.L1_3_write_cnt = new int(0);
   tmp.L1_3_read_cnt = new int(0);
   tmp.L2_write_cnt = new int(0);
   tmp.L2_read_cnt = new int(0);
   tmp.maxlifetime = new UInt64(0);
   tmp.approx_add_cnt = new int(0);
   tmp.approx_mul_cnt = new int(0);
	//poten values
   tmp.poten_L1_write_cnt = new int(0) ;
   tmp.poten_L1_read_cnt = new int(0);
   tmp.poten_L1_1_write_cnt = new int(0);
   tmp.poten_L1_1_read_cnt = new int(0);  
   tmp.poten_L1_2_write_cnt = new int(0);
   tmp.poten_L1_2_read_cnt = new int(0);   
   tmp.poten_L1_3_write_cnt = new int(0);
   tmp.poten_L1_3_read_cnt = new int(0);
   tmp.poten_L2_write_cnt = new int(0);
   tmp.poten_L2_read_cnt = new int(0);
   tmp.poten_maxlifetime = new UInt64(0);	
   tmp.poten_approx_add_cnt = new int(0);
   tmp.poten_approx_mul_cnt = new int(0);

   //std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
   std::pair<std::set<appvar,appcmp>::iterator,bool> it = approx_vars.insert(tmp);
   if(it.second)				//(it == approx_vars.end())		//new value
   {
	   if(address > end_addr)	end_addr = address;
	   if(address < start_addr)	start_addr = address;
      //approx_vars.insert(tmp);
      //std::cout << "approx address in setnum: " << address << "\t" << set_number <<std::endl;
      app_cnter++;
      return true;
   }
   else
      return false;
}

bool MagicServer::del_approx(UInt64 address)			//expects byte address
{
	//std::cout << "inside del_approx!" << std::endl;
   appvar tmp; tmp.addr = address;
   std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
   if(it == approx_vars.end())
      return false;  //value not available
   else
   {
	   //std::cout << "approx address in setnum: " << address << "\t" << set_number <<std::endl;
      approx_vars.erase(it);
      return true;
   }
}

bool MagicServer::is_approx(UInt64 address)			//expects byte address
{
   appvar tmp; tmp.addr = address;
   std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
   if(it == approx_vars.end())
		return false;
   else
		return true;	
}

bool MagicServer::is_block_approx(UInt64 address,UInt32 block_size)
{
	for (unsigned int i=0; i<block_size; i++)
	{
		if(is_approx(address+i))
			return true;
	}
	return false;
}

int MagicServer::approx_addr_setnum(UInt64 address)			//expects byte address
{
   appvar tmp; tmp.addr = address;
   std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
   if(it == approx_vars.end())
		return -1;
   else
		return (int)*(it->set_num);	
}

int MagicServer::approx_block_setnum(UInt64 address,UInt32 block_size)
{
	for (unsigned int i=0; i<block_size; i++)
	{
		int setnum = approx_addr_setnum(address+i);
		if(setnum != -1)
			return setnum;
	}
	return -1;
}



void MagicServer::add_read_acc(UInt64 address, core_id_t cid, std::string cachelvl, UInt32 block_size) //always get block addr
{	
	if(address+block_size < start_addr || address > end_addr)				
	   return;
   //std::cout << "inside read access increment" << std::endl;
   for(UInt32 i=0; i<block_size; i++)
   {
		appvar tmp; tmp.addr = address+i;
		std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
		if(it == approx_vars.end())
			return;	
		it->inc_read_cnt(cid,cachelvl);
		/*
		if(cachelvl == "L1-D" || cachelvl == "l1d")
        {
			if(cid == 0)
                *((it)->poten_L1_read_cnt) = *((it)->poten_L1_read_cnt)+1;
            else if(cid == 1)
                *((it)->poten_L1_1_read_cnt) = *((it)->poten_L1_1_read_cnt)+1;
            else if(cid == 2)
                *((it)->poten_L1_2_read_cnt) = *((it)->poten_L1_2_read_cnt)+1;
            else if(cid == 3)
                *((it)->poten_L1_3_read_cnt) = *((it)->poten_L1_3_read_cnt)+1;			
            else
                std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;
        }
		if(cachelvl == "L2" || cachelvl == "l2")
			*(it->poten_L2_read_cnt) = *(it->poten_L2_read_cnt)+1;   
		*/
#if DEBUG			    //WARNING: not printing the second core
			if(tmp.addr == 134520980 || tmp.addr == 134555905)
				std::cout << ">>> add read access for " << tmp.addr << " inside " << cachelvl << " " 
				<< *(it->poten_L1_read_cnt)  << " " << *(it->poten_L1_write_cnt)  << " " << *(it->poten_L2_read_cnt)  << " " << *(it->poten_L2_write_cnt)  << " >>> " 
				<< *(it->L1_read_cnt)  << " " << *(it->L1_write_cnt)  << " " << *(it->L2_read_cnt)  << " " << *(it->L2_write_cnt)<< std::endl;
#endif				
   }
   

}

void MagicServer::add_write_acc(UInt64 address, core_id_t cid, std::string cachelvl, UInt32 block_size) //always get block addr
{		
	if(address+block_size < start_addr || address > end_addr)				
	   return;
   //std::cout << "inside write access increment" << std::endl;   
   for(UInt32 i=0; i<block_size; i++)
   {
		appvar tmp; tmp.addr = address+i;
		std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
		if(it == approx_vars.end())
			return;
		it->inc_write_cnt(cid,cachelvl);
		/*
		if(cachelvl == "L1-D" || cachelvl == "l1d")
        {   
            if(cid == 0)
                *(it->poten_L1_write_cnt) = *(it->poten_L1_write_cnt)+1;
            else if(cid == 1)
                *(it->poten_L1_1_write_cnt) = *(it->poten_L1_1_write_cnt)+1;
            else if(cid == 2)
                *(it->poten_L1_2_write_cnt) = *(it->poten_L1_2_write_cnt)+1;
            else if(cid == 3)
                *(it->poten_L1_3_write_cnt) = *(it->poten_L1_3_write_cnt)+1;			
            else
                std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;                
        }
		if(cachelvl == "L2" || cachelvl == "l2" )
			*(it->poten_L2_write_cnt) = *(it->poten_L2_write_cnt)+1;  
		*/
#if DEBUG			    // WARNING: as others
			if(tmp.addr == 134520980 || tmp.addr == 134555905)
				std::cout << ">>> add write access for " << tmp.addr << " inside " << cachelvl << " " 
				<< *(it->poten_L1_read_cnt)  << " " << *(it->poten_L1_write_cnt)  << " " << *(it->poten_L2_read_cnt)  << " " << *(it->poten_L2_write_cnt)  << " >>> " 
				<< *(it->L1_read_cnt)  << " " << *(it->L1_write_cnt)  << " " << *(it->L2_read_cnt)  << " " << *(it->L2_write_cnt)<< std::endl;
#endif				
   }
   
}



bool MagicServer::update_liftime(UInt64 address,UInt64 now,UInt64 lpacc,std::set<UInt64>* rav,bool RW, UInt32 block_size)     //return true if address is approx
{												//expects address within page and updates for each byte in page
   // SInt64 snow = (SInt64)now;
   // SInt64 slpacc = (SInt64)lpacc;
   UInt64 inter;
   if(now >= lpacc)
      inter = now-lpacc;
   else
      inter = lpacc-now;
   for(UInt64 addrval=(address/PAGE_SIZE)*PAGE_SIZE; addrval<(address/PAGE_SIZE+1)*PAGE_SIZE; addrval++)    //update all approxes in page
   {
      appvar tmp; tmp.addr = addrval;
      std::set<appvar,appcmp>::iterator itapprox=approx_vars.find(tmp);     
      if(itapprox != approx_vars.end())    //addr is approx
      {
         if(inter > *(itapprox->poten_maxlifetime))
         {
            *(itapprox->poten_maxlifetime) = inter;
         }
            //std::cout << "inter, poten_maxlifetime, " << inter << "\t\t" << *(itapprox->poten_maxlifetime) << std::endl;
         //std::cout << "yes" << std::endl;
      }
   }
   if(address+block_size < start_addr || address > end_addr)
	   return false;
   bool realend = false;   
   appvar tmp; tmp.addr = address;
   std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
   for(unsigned int i=0;i<=block_size;i++)
   {
	   if(it == approx_vars.end())
	   {
		  if(realend == false)
		  {
			tmp.addr = address+i; it=approx_vars.find(tmp);
			continue;  //addr not approximate or really the end	
		  }
		  else
			return true;  //really the end		   
	   }
	   else if(it->addr >= address && it->addr < address+block_size)		//address exists
	   {
			realend = true;
		   // normal updata stuff		   
		  if(RW && *(it->maxlifetime) < *(it->poten_maxlifetime))
		  {
			 *(it->maxlifetime) = *(it->poten_maxlifetime);     //setting new max
		  }
		  else if(!RW && *(it->maxlifetime) < *(it->poten_maxlifetime))
		  {
			 *(it->poten_maxlifetime) = *(it->maxlifetime) ;    //invalidating
		  }
		  // end of normal updata stuff
	   }
	   it++;
   }
   return true;
}

/*
	appvar tmp; tmp.addr = address;
	std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp); 
	if(it != approx_vars.end())
	{
		   // normal updata stuff		   
		  if(RW && *(it->maxlifetime) < *(it->poten_maxlifetime))
		  {
			 *(it->maxlifetime) = *(it->poten_maxlifetime);     //setting new max
		  }
		  else if(!RW && *(it->maxlifetime) < *(it->poten_maxlifetime))
		  {
			 *(it->poten_maxlifetime) = *(it->maxlifetime) ;    //invalidating
		  }
		  // end of normal updata stuff		
	}
	return true;
*/

void MagicServer::reset_cnters(UInt64 address, UInt32 len)		//expects byte address
{
	for(UInt32 i=0; i<len; i++)
	{
		appvar tmp; tmp.addr = address+i;
		std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp); 	
		if(it != approx_vars.end())	//address exits
		{ 
			it->reset();
			//std::cout << " inside reset_cnters" << std::endl;
			/*
			*(it->poten_L1_write_cnt)	= 0;
			*(it->poten_L1_read_cnt)	= 0;
			*(it->poten_L1_1_write_cnt)	= 0;
			*(it->poten_L1_1_read_cnt)	= 0;
			*(it->poten_L1_2_write_cnt)	= 0;
			*(it->poten_L1_2_read_cnt)	= 0;
			*(it->poten_L1_3_write_cnt)	= 0;
			*(it->poten_L1_3_read_cnt)	= 0;
			*(it->poten_L2_write_cnt)	= 0;
			*(it->poten_L2_read_cnt)	= 0; 	
			*/
#if DEBUG			        // WARNING: not printing L1_1 values
			if(tmp.addr == 134520980 || tmp.addr == 134555905)
				std::cout << ">>> reseting cnters for " << tmp.addr <<  " " 
				<< *(it->poten_L1_read_cnt)  << " " << *(it->poten_L1_write_cnt)  << " " << *(it->poten_L2_read_cnt)  << " " << *(it->poten_L2_write_cnt)  << " >>> " 
				<< *(it->L1_read_cnt)  << " " << *(it->L1_write_cnt)  << " " << *(it->L2_read_cnt)  << " " << *(it->L2_write_cnt)<< std::endl;
#endif				
		}
	}
    
}	

void MagicServer::upsave_cnters(UInt64 address, UInt32 len)		//expects byte address
{
	for(UInt32 i=0; i<len; i++)
	{
		appvar tmp; tmp.addr = address+i;
		std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp); 
		//std::cout << "Address " << address << (it != approx_vars.end() ? " is approx": "") << std::endl;
		if(it != approx_vars.end())	//approx address exits
		{ 
			it->upsave();
			//std::cout << " inside upsave_cnters" << std::endl;
			//int pre_cnt = *(it->L1_write_cnt) + *(it->L2_write_cnt) + *(it->L1_read_cnt) + *(it->L2_read_cnt);
			//int cur_cnt = *(it->poten_L1_write_cnt) + *(it->poten_L2_write_cnt) + *(it->poten_L1_read_cnt) + *(it->poten_L2_read_cnt);
			/*
			*(it->L1_write_cnt)	= MAX(*(it->poten_L1_write_cnt)	,*(it->L1_write_cnt));
			*(it->L1_read_cnt)	= MAX(*(it->poten_L1_read_cnt)	,*(it->L1_read_cnt)	);
			*(it->L1_1_write_cnt)	= MAX(*(it->poten_L1_1_write_cnt)	,*(it->L1_1_write_cnt));
			*(it->L1_1_read_cnt)	= MAX(*(it->poten_L1_1_read_cnt)	,*(it->L1_1_read_cnt)	);
			*(it->L1_2_write_cnt)	= MAX(*(it->poten_L1_2_write_cnt)	,*(it->L1_2_write_cnt));
			*(it->L1_2_read_cnt)	= MAX(*(it->poten_L1_2_read_cnt)	,*(it->L1_2_read_cnt)	);
			*(it->L1_3_write_cnt)	= MAX(*(it->poten_L1_3_write_cnt)	,*(it->L1_3_write_cnt));
			*(it->L1_3_read_cnt)	= MAX(*(it->poten_L1_3_read_cnt)	,*(it->L1_3_read_cnt)	);			
			*(it->L2_write_cnt)	= MAX(*(it->poten_L2_write_cnt)	,*(it->L2_write_cnt));
			*(it->L2_read_cnt)	= MAX(*(it->poten_L2_read_cnt)	,*(it->L2_read_cnt)	); 	
			*/
#if DEBUG			     // WARNING: not printing L1_1 values
			if(tmp.addr == 134520980 || tmp.addr == 134555905)
				std::cout << ">>> upsaveing cnters for " << tmp.addr <<  " " 
				<< *(it->poten_L1_read_cnt)  << " " << *(it->poten_L1_write_cnt)  << " " << *(it->poten_L2_read_cnt)  << " " << *(it->poten_L2_write_cnt)  << " >>> " 
				<< *(it->L1_read_cnt)  << " " << *(it->L1_write_cnt)  << " " << *(it->L2_read_cnt)  << " " << *(it->L2_write_cnt)<< std::endl;
#endif				
		}   
	}
}

void MagicServer::display_approx(void)
{

               //std::cout << "ADDR \t\tCORE \t\tLEVEL \t\tREADS \t\tWRITES \t\tLIFETIME" << std::endl;
               std::set<appvar,appcmp>::iterator it;
               for(it=approx_vars.begin(); it != approx_vars.end(); ++it)
               {
				   std::cout << "approx address in setnum: " << it->addr << "\t" << *(it->set_num) <<std::endl;
/*
                  for(int i=0; i<(int)it->caches->size();i++)
                  {
                     std::cout << it->addr << " \t\t" 
                        << it->caches->at(i).coreid   << " \t\t" 
                        << it->caches->at(i).cachelvl << " \t\t"
                        << it->caches->at(i).Rcounter << " \t\t" 
                        << it->caches->at(i).Wcounter << " \t\t"
                        << *(it->maxlifetime) <<   std::endl;
			         std::cout << it->addr << " \t\t" 
                        << "0"   << " \t\t" 
                        << "L1-D" << " \t\t"
                        << *(it->L1_read_cnt) << " \t\t" 
                        << *(it->L1_write_cnt) << " \t\t"
                        << *(it->maxlifetime) <<   std::endl;
			         std::cout << it->addr << " \t\t" 
                        << "0"   << " \t\t" 
                        << "L2" << " \t\t"
                        << *(it->L2_read_cnt) << " \t\t" 
                        << *(it->L2_write_cnt) << " \t\t"
                        << *(it->maxlifetime) <<   std::endl;						
				  }	
*/	                  
               }
                 	
	
}
void MagicServer::add_pure_acc(UInt64 addr,int corenum,std::string cachelvl,bool read )
{
	appvar tmp; tmp.addr = addr;
	std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);
	if(it == approx_vars.end())
		return;
	bool write = !read; 
	if((cachelvl == "L1-D" || cachelvl == "l1d") && read)
    {
        if(corenum == 0)
            *(it->L1_read_cnt) = *(it->L1_read_cnt)+1;
        else if(corenum == 1)
            *(it->L1_1_read_cnt) = *(it->L1_1_read_cnt)+1;
        else if(corenum == 2)
            *(it->L1_2_read_cnt) = *(it->L1_2_read_cnt)+1;
		else if(corenum == 3)
            *(it->L1_3_read_cnt) = *(it->L1_3_read_cnt)+1;
        else
            std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;
    }
	if((cachelvl == "L1-D" || cachelvl == "l1d") && write)
    {
        if(corenum == 0)
            *(it->L1_write_cnt) = *(it->L1_write_cnt)+1;
        else if(corenum == 1)
            *(it->L1_1_write_cnt) = *(it->L1_1_write_cnt)+1;
        else if(corenum == 2)
            *(it->L1_2_write_cnt) = *(it->L1_2_write_cnt)+1;
		else if(corenum == 3)
            *(it->L1_3_write_cnt) = *(it->L1_3_write_cnt)+1;
        else
            std::cout << "ERROR: invalid core number (magic_server.cc)" << std::endl;
    }
	if((cachelvl == "L2" || cachelvl == "l2" ) && read)
		*(it->L2_read_cnt) = *(it->L2_read_cnt)+1;
	if((cachelvl == "L2" || cachelvl == "l2") && write)
		*(it->L2_write_cnt) = *(it->L2_write_cnt)+1;
}

/*
   SInt64 snow = (SInt64)now;
   SInt64 slpacc = (SInt64)lpacc;
   appvar tmp; tmp.addr = address;
   std::set<appvar,appcmp>::iterator it=approx_vars.find(tmp);   
   if(it == approx_vars.end())
      return false;  //addr not approximate
   else
   {
      if(now >= lpacc)   //everything is normal
      {
         UInt64 inter = now - lpacc;
         if(inter > *(it->maxlifetime))      //new maxtime
         {
            *(it->lastmaxlifetime) = *(it->maxlifetime);
            *(it->maxlifetime) = inter;
            *(it->maxsttime) = now;
         }
      }
      else if(slpacc-(SInt64)*(it->maxlifetime) == (SInt64)*(it->maxsttime))      //max is for this interval
      {
            std::cout << "how many times this happens?" << std::endl;
         if(snow > slpacc-(SInt64)*(it->maxlifetime))    //invalidate max
         {
            *(it->maxsttime) = now;    //setting max
            UInt64 it2 = lpacc-now;
            UInt64 it1 = *(it->maxlifetime)-it2;
            UInt64 it3 = *(it->lastmaxlifetime);
            if(it1 >= it3 && it1 >= it2)
               *(it->maxlifetime) = it1;
            else if(it2 >= it1 && it2 >= it3)
               *(it->maxlifetime) = it2;
            else if(it3 >= it1 && it3 >= it1)
               *(it->maxlifetime) = it3;
         }
      }
      return true;
   }
*/
