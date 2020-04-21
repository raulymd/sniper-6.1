/*
 * flty.h
 * 
 * Created on: Nov 6, 2017
 * 			Author: Raul Yarmand 
 * Version: 1.0
 */
#ifndef __FLTY_H__
#define __FLTY_H__

#include <random> 
#include <ctime>

#ifndef FAULT_PROB
	#define FAULT_PROB 0
#endif

static std::default_random_engine generator(time(NULL));
static std::bernoulli_distribution distribution(FAULT_PROB);

template <class T>
T flty(T in)	
{	//making value faulty	
	T out = in;		
	for(int i=0;i <(8*sizeof(T));i++)
	{	
		// up to now only toggle mode supported
		if(distribution(generator))
			out ^= 1 << i;
	}
	return out;
}

#endif