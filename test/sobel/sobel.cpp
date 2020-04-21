/*
 * sobel.cpp
 * 
 * Created on: Dec 1, 2018
 * 			Author: Raul Ymd <raul.ymd@gmail.com>
 *
 *
 *	
 *	image sobelening algorithm by 5*5 sobel filter
 *
 */


#define ULONG unsigned long
#define UCHAR unsigned char
#define UINT8 unsigned char

#include <iostream>
#include <cmath>
#include <fstream>

#define WIDHEI 16
#include "/home/shahdaad/raul/images/hpp/peppers_gray.hpp"          //address of hpp image
//#include IMHEADER

#ifdef SNIPER
	#include "sim_api.h"
#endif 

#ifdef GEM5     //gem5 is only used for fault simulation
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
#endif

#define BIT_LENGTH 64

#ifdef GEM5
long rapcla(long a, long b, long w)		//w stands for window
{
	bool A[BIT_LENGTH-1];
	bool B[BIT_LENGTH-1];
	bool P[BIT_LENGTH-1];
	bool G[BIT_LENGTH-1];
	bool C[BIT_LENGTH-1] = {0};
	bool S[BIT_LENGTH-1];
	for(int i=0; i<BIT_LENGTH-1; i++)
	{
		long t = 1 << i;
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
	
	long s = 0;
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
        default:    printf("ERROR: insxid approximate multiplier number (magic_server.cc)");
        return 0;
    }                     
    
    if(a*b < 0)
        return -1*res;
    else
        return res;
}
#endif

/*
int saveRgbImage(const char* fileName) {		
	int i;
	int j;
	FILE *fp;
	fp = fopen(fileName, "w");
	if (!fp) {
		printf("[AMN] Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}
	fprintf(fp, "%d,%d\n", WIDHEI, WIDHEI);
	for(i = 0; i < WIDHEI; i++) {
		for(j = 0; j < (WIDHEI - 1); j++) {
			fprintf(fp, "%d,%d,%d,", int((dstImage[i][j][0])), int((dstImage[i][j][1])), int((dstImage[i][j][2])));
		}
		fprintf(fp, "%d,%d,%d\n", int((dstImage[i][j][0])), int((dstImage[i][j][1])), int((dstImage[i][j][2])));
	}

	fprintf(fp, "Manipulated by Raul Ymd");
	fclose(fp);
	return 1;
} 
*/

int saveChImage(const char* fileName) {		
	int i;
	int j;
	FILE *fp;
	fp = fopen(fileName, "w");
	if (!fp) {
		printf("[SOB] Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}
	
    fprintf(fp,"%d\n",WIDHEI*WIDHEI*3);
    fwrite(dstImage , sizeof(char), sizeof(dstImage), fp);

	fclose(fp);
	return 1;
} 

#ifdef GEM5
void write_vaddrs(const char* num)
{
    FILE *fp;
    std::string fname = "vaddrs_" + (std::string)num + ".txt";
    fp = fopen(fname.c_str(),"w");      //NOTE: pay attention to format
    fprintf(fp,"%lx\n",(ULONG)&psnr);
    fprintf(fp,"%lx\n",(ULONG)srcImage);
    fprintf(fp,"%lx\n",(ULONG)dstImage);
    fprintf(fp,"%d\n",WIDHEI);
    fclose(fp);
}
#endif


int main(int argc, const char* argv[]) 
{
	//clock_t begin = clock();
    int x, y;
    int i, j;
    int n, sx, sy, s;   
    
#ifdef GEM5
    if(argc == 1)
        write_vaddrs("0");
    else
        write_vaddrs(argv[1]); 
#endif	   

#ifdef SNIPER
	SimRoiStart();
	//marking variables as approximate -> Raul Codes
	SimAddApproxAddrs((ULONG)&srcImage[0][0][0],WIDHEI*WIDHEI*3);
	SimAddApproxAddrs((ULONG)&dstImage[0][0][0],WIDHEI*WIDHEI*3);
#endif    
    
    n= 3;       //sobel filter size
    double sobel_x_mask[n][n] =
		{
			{ -1, -2, -1},
			{  0,  0,  0},
			{  1,  2,  1}
		} ;
        
    double sobel_y_mask[n][n] =
		{
			{ -1,  0,  1},
			{ -2,  0,  2},
			{ -1,  0,  1}
		} ;        
		
	//makeGrayscale(srcImage);    
    for(x=0; x<srcImageWidth; x++)
    {
        for(y=0; y<srcImageWidth; y++)
        {
            sx = 0; sy = 0;
            for (i=0; i<n; i++)
            {
                for (j=0; j<n; j++)
                {
                    int zx = x+i-n/2;
                    int zy = y+j-n/2;
                    if(zx >= 0 && zy >= 0 && zx<srcImageWidth && zy<srcImageWidth)
                        
#ifdef GEM5       
                        sx = rapcla(sx,appmul(srcImage[zx][zy][0],sobel_x_mask[i][j],19),16);       //WARNING: the image must be grayscale
                        sy = rapcla(sy,appmul(srcImage[zx][zy][0],sobel_y_mask[i][j],19),16);       //WARNING: the image must be grayscale                        sy = sy + (srcImage[zx][zy][0])*sobel_y_mask[i][j];            //WARNING: the image must be grayscale
#else
                        sx = sx + (srcImage[zx][zy][0])*sobel_x_mask[i][j];            //WARNING: the image must be grayscale
                        sy = sy + (srcImage[zx][zy][0])*sobel_y_mask[i][j];            //WARNING: the image must be grayscale
#endif  
                    
                }
            }
            s = sqrt(sx * sx + sy * sy) ;       //approximate add and mul can be added.
            if (s >= (256* 256 / sqrt(256 * 256 + 256 * 256)))
                s = 256* 255 / sqrt(256 * 256 + 256 * 256);
            dstImage[x][y][0] = s;
            dstImage[x][y][1] = s;
            dstImage[x][y][2] = s;
            
#ifdef SNIPER        
		//adding ADD counters based on algorithm
		SimApproxAddCnt((ULONG)&dstImage[x][y][0],16);
		SimApproxAddCnt((ULONG)&dstImage[x][y][1],16);
		SimApproxAddCnt((ULONG)&dstImage[x][y][2],16);

		SimApproxMulCnt((ULONG)&dstImage[y][x][0],18);
		SimApproxMulCnt((ULONG)&dstImage[y][x][1],18);
		SimApproxMulCnt((ULONG)&dstImage[y][x][2],18);	         
#endif  
            
        }
    }
    //saveRgbImage( "sobel_outimg.rgb");
    if(argc == 1)
        saveChImage( "sobel_outimg_0.ch");		//because saveChImage also contributes in fault and DataSig
    else
    {
        std::string fname = (std::string)"sobel_outimg_" + (std::string)argv[1] + ".ch";
        saveChImage(fname.c_str());		
    }
    
#ifdef SNIPER
	SimDelApproxAddrs((ULONG)&srcImage[0][0][0],WIDHEI*WIDHEI*3);
	SimDelApproxAddrs((ULONG)&dstImage[0][0][0],WIDHEI*WIDHEI*3);
	SimRoiEnd();
	//deleting variables as approximate -> Raul Codes
#endif	    
	
	//clock_t end = clock();
	//double time_spent = (double)(end - begin) ;
	//printf("execution time is: %f\n",time_spent);
	
    return  0;
}
