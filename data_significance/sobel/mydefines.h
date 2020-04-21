//#define WIDHEI 512

#define UINT8 unsigned char
#define UCHAR unsigned char
#define ULONG unsigned long
#define DEBUG 0

#ifdef 	DATA_SIG
	#include "../flty.h"
	#define FLT(AAA) flty<UINT8>(AAA)		
#else
	#define FLT(AAA) (AAA)
#endif