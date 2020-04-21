/*
 * sobel.cpp
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 *
 *	Annotated by: Raul Yarmand
 *	to add approximate variables for both sniper and data significance analysis 
 */

#define ULONG unsigned long

#include "convolution.hpp"
#include <iostream>
#include <cmath>
#include <fstream>

#include "mydefines.h"
#include IMHEADER
#include <cmath>



int saveChImage(const char* fileName) {		
	int i;
	int j;
	FILE *fp;
	fp = fopen(fileName, "w");
	if (!fp) {
		printf("[SOB] Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}
#ifdef DATA_SIG
    for(int x=0; x<WIDHEI; x++)
    {
        for(int y=0; y<WIDHEI; y++)
        {
            dstImage[x][y][0] = FLT(dstImage[x][y][0]);
            dstImage[x][y][1] = FLT(dstImage[x][y][1]);
            dstImage[x][y][2] = FLT(dstImage[x][y][2]);
        }
    }
#endif      
    fprintf(fp,"%d\n",WIDHEI*WIDHEI*3);
    fwrite(dstImage , sizeof(char), sizeof(dstImage), fp);

	fclose(fp);
	return 1;
} 


int saveRgbImage(const char* fileName) 	//dstImage  must be used instead of srcImage for sobel
{		
	int i;
	int j;
	FILE *fp;
	fp = fopen(fileName, "w");
	if (!fp) {
		printf("[SOB] Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}
	fprintf(fp, "%d,%d\n", WIDHEI, WIDHEI);
	for(i = 0; i < WIDHEI; i++) {
		for(j = 0; j < (WIDHEI - 1); j++) {
			fprintf(fp, "%d,%d,%d,", int(FLT(dstImage[i][j][0])), int(FLT(dstImage[i][j][1])), int(FLT(dstImage[i][j][2])));
		}
		fprintf(fp, "%d,%d,%d\n", int(FLT(dstImage[i][j][0])), int(FLT(dstImage[i][j][1])), int(FLT(dstImage[i][j][2])));
	}

	fprintf(fp, "Manipulated by Raul Ymd");
	fclose(fp);
	return 1;
}


void makeGrayscale(UCHAR image[][WIDHEI][3])        //can be copied by binarization.c
{

	float luminance ;

	float rC = 0.30  ;		
	float gC = 0.59  ;		
	float bC = 0.11  ;		

	for(int w = 0 ; w < WIDHEI ; w++)
	{
		for(int h = 0 ; h < WIDHEI ; h++)
		{
			luminance = 	   (( rC * FLT((UCHAR)image[h][w][0]) ) + 		
								( gC * FLT((UCHAR)image[h][w][1]) ) +         
								( bC * FLT((UCHAR)image[h][w][2]) )) ;        

			image[h][w][0] = (UCHAR)(luminance + 0.0001);
			image[h][w][1] = (UCHAR)(luminance + 0.0001); 
			image[h][w][2] = (UCHAR)(luminance + 0.0001); 
		}
	}
}



int main ( int argc, const char* argv[])
{
	//clock_t begin = clock();
	int x, y;
	float s = 0;
    float w[][3] = {
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}
	};
    
    
	y = 0 ;
	// Start performing Sobel operation
	for( x = 0 ; x < srcImageWidth ; x++ ) {
		HALF_WINDOW(srcImage, x, y, w) ;


			s = sobel(w);


		dstImage[y][x][0] = (UCHAR)s ;
		dstImage[y][x][1] = (UCHAR)s ;
		dstImage[y][x][2] = (UCHAR)s ;
        
	}
    //return 0;       //first row
    
	for (y = 1 ; y < (srcImageHeight - 1) ; y++) {
		x = 0 ;
		HALF_WINDOW(srcImage, x, y, w);

			s = sobel(w);

		dstImage[y][x][0] = (UCHAR)s ;
		dstImage[y][x][1] = (UCHAR)s ;
		dstImage[y][x][2] = (UCHAR)s ;    
        

		for( x = 1 ; x < srcImageWidth - 1 ; x++ ) {
 

            
			WINDOW(srcImage, x, y, w) ;
				
				s = sobel(w);

			dstImage[y][x][0] = (UCHAR)s ;
			dstImage[y][x][1] = (UCHAR)s ;
			dstImage[y][x][2] = (UCHAR)s ;
			
		}

		x = srcImageWidth - 1 ;
		HALF_WINDOW(srcImage, x, y, w) ;
		

			s = sobel(w);

		dstImage[y][x][0] = (UCHAR)s ;
		dstImage[y][x][1] = (UCHAR)s ;
		dstImage[y][x][2] = (UCHAR)s ;
   
	}

	y = srcImageHeight - 1;

	for(x = 0 ; x < srcImageWidth ; x++) {
		HALF_WINDOW(srcImage, x, y, w) ;
		
			s = sobel(w);

		dstImage[y][x][0] = (UCHAR)s ;
		dstImage[y][x][1] = (UCHAR)s ;
		dstImage[y][x][2] = (UCHAR)s ;
          

	}
	
    if(argc == 1)
        saveChImage( "sobel_outimg_0.ch");
    else 
    {
        std::string fname = (std::string)"sobel_outimg_" + (std::string)argv[1] + ".ch";
        saveChImage(fname.c_str());		
    }
	
	//clock_t end = clock(); 
	//double time_spent = (double)(end - begin) ;
	//printf("execution time is: %f\n",time_spent);	

	return 0 ;
}