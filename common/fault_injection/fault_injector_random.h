#ifndef __FAULT_INJECTOR_RANDOM_H
#define __FAULT_INJECTOR_RANDOM_H

#include "fault_injection.h"
#include <random>

class FaultInjectorRandom : public FaultInjector
{
   public:
      FaultInjectorRandom(UInt32 core_id, MemComponent::component_t mem_component);
	  
	  //Raul
	  float read_prob;
	  float hold_prob;
	  float write_prob;
 
	  std::bernoulli_distribution*	rand_numR;
      std::bernoulli_distribution*	rand_numH;
      std::bernoulli_distribution*	rand_numW;
	  
	  std::bernoulli_distribution*	rand_numR_2;
      std::bernoulli_distribution*	rand_numH_2;
      std::bernoulli_distribution*	rand_numW_2;	 

	  std::default_random_engine generatorR;
	  std::default_random_engine generatorH;  
      std::default_random_engine generatorW;
      std::default_random_engine generatorR_2;
      std::default_random_engine generatorH_2;  
	  std::default_random_engine generatorW_2;
	  //std::default_random_engine generator;
	  
	  virtual void preRead(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time);
      virtual void postWrite(IntPtr addr, IntPtr location, UInt32 data_size, Byte *fault, SubsecondTime time);

   private:
      bool m_active;
      UInt64 m_rng;
};

#endif // __FAULT_INJECTION_RANDOM_H
