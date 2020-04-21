/*
 * convolution.hpp
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 *
 *	Annotated by: Raul Yarmand
 *	to add approximate variables for both sniper and data significance analysis 
 */

 #ifndef __CONVOLUTION_HPP__
 #define __CONVOLUTION_HPP__

#include <iostream>
#include <string>

#include "mydefines.h"



 float sobel(float w[][3]) ;
 #define WINDOW(image, x, y, window) {					\
 	window[0][0] = FLT(image[ y - 1 ][ x - 1 ][0]) ;	\
 	window[0][1] = FLT(image[ y - 1 ][ x ][0]	) ; \
 	window[0][2] = FLT(image[ y - 1 ][ x + 1 ][0]) ;	\
																\
	window[1][0] = FLT(image[ y ][ x - 1 ][0]	);	\
 	window[1][1] = FLT(image[ y ][ x ][0]	    );	\
 	window[1][2] = FLT(image[ y ][ x + 1 ][0]	);	\
																\
	window[2][0] = FLT(image[ y + 1 ][ x - 1 ][0]) ;	\
 	window[2][1] = FLT(image[ y + 1 ][ x ][0] 	) ; \
 	window[2][2] = FLT(image[ y + 1 ][ x + 1 ][0]) ;	\
}

#define HALF_WINDOW(image, x, y, window) {																			\
	window[0][0] = (x == 0 || y == 0										) ? 0 : FLT(image[y - 1][x - 1][0]);	\
	window[0][1] = (y == 0													) ? 0 : FLT(image[y - 1][x][0]	 );	\
	window[0][2] = (x == srcImageWidth -1 || y == 0						) ? 0 : FLT(image[y - 1][x + 1][0]);	\
																															\
	window[1][0] = (x == 0 													) ? 0 : FLT(image[y][x - 1][0]	 );	\
	window[1][1] = 																	FLT(image[y][x][0]		 );	\
	window[1][2] = (x == srcImageWidth -1									) ? 0 : FLT(image[y][x + 1][0]	 );	\
																															\
	window[2][0] = (x == 0 || y == srcImageHeight - 1						) ? 0 : FLT(image[y + 1][x - 1][0]);	\
	window[2][1] = (y == srcImageHeight - 1								) ? 0 : FLT(image[y + 1][x][0]	 );	\
	window[2][2] = (x == srcImageWidth -1 || y == srcImageHeight - 1	) ? 0 : FLT(image[y + 1][x + 1][0]);	\
}


 #endif