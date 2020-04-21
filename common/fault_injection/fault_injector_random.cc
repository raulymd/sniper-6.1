#include "fault_injector_random.h"
#include "rng.h"
#include <iostream>
#include <fstream>
#include <random>
#include "simulator.h"
#include "magic_server.h"
#include <time.h>


#define ONCE(AAAA) { static bool once(true); if(once){ once = false; AAAA}}

void Raul_GetProb(UInt32 core_id, MemComponent::component_t mem_component,float &read_prob,float &hold_prob,float &write_prob, int setnum)
{
   read_prob = 0;
   hold_prob = 0;
   write_prob = 0;   
   float scanned_read_prob;
   float scanned_hold_prob;
   float scanned_write_prob;
   UInt32 scanned_core_id;
   std::string scanned_mem_comp;
   std::ifstream ifs;
   if(setnum == 0)
      ifs.open("faultProbabilities.txt", std::ifstream::in);
   else if(setnum == 1)
      ifs.open("faultProbabilities_2.txt", std::ifstream::in);
   if(ifs.is_open())
   {
      while(ifs.good()){
         ifs >> scanned_core_id >> scanned_mem_comp >> scanned_read_prob >> scanned_hold_prob >> scanned_write_prob;
         if(scanned_core_id == core_id && scanned_mem_comp == MemComponentString(mem_component)){
            read_prob = scanned_read_prob;
            hold_prob = scanned_hold_prob;
            write_prob = scanned_write_prob;
             // std::cout << "[FAULT] value scanned for core " << scanned_core_id << " and " << scanned_mem_comp << ", " 
             // << read_prob << " " << hold_prob << " " << write_prob << std::endl;
            break;
         }  
      }
   }
   else
   {
      if(setnum == 0) 
		  std::cout << "[!!!!!!] Error opening faultProbabilities.txt" << std::endl;
   }
}

FaultInjectorRandom::FaultInjectorRandom(UInt32 core_id, MemComponent::component_t mem_component)
   : FaultInjector(core_id, mem_component)
   , m_rng(rng_seed(0))
{	
    ONCE(std::cout << "[SNIPER] Fault injection enabled by config file" << std::endl;)
   float read_prob		= 0;   float hold_prob		= 0;   float write_prob		= 0;  
   float read_prob_2	= 0;   float hold_prob_2	= 0;   float write_prob_2	= 0;     
   Raul_GetProb(core_id, mem_component,read_prob	,hold_prob		,write_prob		,0);	
   //Raul_GetProb(core_id, mem_component,read_prob_2	,hold_prob_2	,write_prob_2	,1);	
   
   std::cout << "[FAULT ] value scanned for " << MemComponentString(mem_component) << ", " 
   << read_prob << " " << hold_prob << " " << write_prob << std::endl;
   
   
   rand_numR = new std::bernoulli_distribution(read_prob);
   rand_numH = new std::bernoulli_distribution(hold_prob);
   rand_numW = new std::bernoulli_distribution(write_prob);

   rand_numR_2 = new std::bernoulli_distribution(read_prob_2);
   rand_numH_2 = new std::bernoulli_distribution(hold_prob_2);
   rand_numW_2 = new std::bernoulli_distribution(write_prob_2);
   
   generatorR.seed(time(NULL));
   generatorH.seed(time(NULL));  
   generatorW.seed(time(NULL));
   generatorR_2.seed(time(NULL));
   generatorH_2.seed(time(NULL));
   generatorW_2.seed(time(NULL));
   
}

void
FaultInjectorRandom::preRead(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time)
{
   // std::default_random_engine generatorR(time.getFS());
   // std::cout << "value of random gen = " << rand_num->operator()(mygen) << std::endl;
   
   // Data at virtual address <addr> is about to be read with size <data_size>.
   // <location> corresponds to the physical location (cache line) where the data lives.
   // Update <fault> here according to errors that have accumulated in this memory location.

   // if(Sim()->getFaultinjectionManager()->FaultApplied){
   //    for(UInt32 i=0; i<data_size; i++)
   //       fault[i]=0;
   //    Sim()->getFaultinjectionManager()->FaultApplied = false;
   // }   

   for(unsigned int i=0; i<data_size; i++)
   {
      int setnum = Sim()->getMagicServer()->approx_addr_setnum((UInt64)(addr+i));	   
      if(setnum == 0)
      {   //if the address is approximate
         //std::cout << "fault[0] value: " << (int)fault[0] << std::endl;
         // Note: selecting the type of memory is done by class
         //std::default_random_engine generatorR;
         //std::default_random_engine generatorH;      
         for(unsigned int j=0; j<8; j++)  //8-bit data
         {
            if(rand_numR->operator()(generatorR))
            {
               fault[i] ^= (1 << j);    
               //rand_numR->reset();
               //std::cout << "time: " << time.getFS() << std::endl;
               // std::cout << "[******] READ-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;
            }
            if(rand_numH->operator()(generatorH))
            {
               fault[i] ^= (1 << j);    
               //rand_numH.reset();
               //std::cout << "New value: " << (int)fault[i] << std::endl;
               // std::cout << "[******] HOLD-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;               
            }
         }
      }
      else if(setnum == 1)	  
	  {
         //std::cout << "fault[0] value: " << (int)fault[0] << std::endl;
         // Note: selecting the type of memory is done by class
         //std::default_random_engine generatorR_2;
         //std::default_random_engine generatorH_2;      
         for(unsigned int j=0; j<8; j++)  //8-bit data
         {
            if(rand_numR_2->operator()(generatorR_2))
            {
               fault[i] ^= (1 << j);    
               //rand_numR->reset();
               //std::cout << "time: " << time.getFS() << std::endl;
               // std::cout << "[******] READ-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;
            }
            if(rand_numH_2->operator()(generatorH_2))
            {
               fault[i] ^= (1 << j);    
               //rand_numH.reset();
               //std::cout << "New value: " << (int)fault[i] << std::endl;
               // std::cout << "[******] HOLD-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;               
            }
         }		  
	  }
   }
}

void
FaultInjectorRandom::postWrite(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time)
{
   // Data at virtual address <addr> has just been written to.
   // Update <fault> here according to errors that occured during the writing of this memory location.
   // Copy of Read part  
   // if(Sim()->getFaultinjectionManager()->FaultApplied){
   //    for(UInt32 i=0; i<data_size; i++)
   //       fault[i]=0;
   //    Sim()->getFaultinjectionManager()->FaultApplied = false;
   // }


   for(unsigned int i=0; i<data_size; i++)
   {
      int setnum = Sim()->getMagicServer()->approx_addr_setnum((UInt64)(addr+i));
      if(setnum == 0)
      {   //if the address is approximate
         //fault[0] = 1;
         //return;
         //std::default_random_engine generatorW;
         for(unsigned int j=0; j<8; j++)  //8-bit data
         {
            if(rand_numW->operator()(generatorW)){
               fault[i] ^= (1 << j);    
               //rand_numW->reset();
               // std::cout << "[******] WRIT-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;
            }
         }
      }    
	  else if(setnum == 1)		//second set fault injection
	  {
         //std::default_random_engine generatorW_2;
         for(unsigned int j=0; j<8; j++)  //8-bit data
         {
            if(rand_numW_2->operator()(generatorW_2)){
               fault[i] ^= (1 << j);    
               //rand_numW->reset();
               // std::cout << "[******] WRIT-FAULT-INJECTION: bit position " 
               // << j << " flipped for byte number " << i << " of addr " << addr 
               // << " inside: core " << m_core_id << ", " << MemComponentString(m_mem_component) << std::endl;
            }
         }		  
	  }
   }


}
