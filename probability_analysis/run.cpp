/* 
	DEVELOPED BY: Raul Yarmand
	DESCRIPTION: breaking variable probabilities to each memory in hierarchy 
	one core (approx add, approx mul), one L1, one L2
	NOTE: 	check tech file location
*/


#define _CRT_SECURE_NO_WARNINGS
#define UInt64 unsigned long long 

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <ctime>
#include <fstream>  
#include <sstream>
#include <cmath>
#include <omp.h>
#include <cstdlib>
#include <limits>
#include <cfloat>
#include <libgen.h>

#define DOUBLE_MAX  std::numeric_limits<double>::max()
//#define DEBUG  

using namespace std;


class appvar
{       //change
public:
	UInt64 addr;
    int L1_reads;
    int L1_writes;
    int L2_reads;
    int L2_writes;
	double* maxlifetime;
	int add_cnt;
    int mul_cnt;
	int findRcount(int cnum, std::string clvl) const;
	int findWcount(int cnum, std::string clvl) const;
};

int appvar::findRcount(int cnum, std::string clvl) const
{
    if(cnum == 0 && clvl == "L1-D")
        return L1_reads;
    if(clvl == "L2")
        return L2_reads;
    return 0;
}

int appvar::findWcount(int cnum, std::string clvl) const
{
    if(cnum == 0 && clvl == "L1-D")
        return L1_writes;
    if(clvl == "L2")
        return L2_writes;
    return 0;
}

struct appcmp {
	bool operator() (const appvar& lhs, const appvar& rhs) const
	{
		return lhs.addr<rhs.addr;
	}
};

void print_time()
{
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	std::cout << "Current local time and date: " << asctime(timeinfo) << std::endl;
}

struct apppack
{
	double appknob;
	long double prob;
};

std::vector<apppack> load_probvals(std::string fname)
{
	std::vector<apppack> myvec;

	//fname = TECHFILES_PATH + fname;
	std::ifstream ifs(fname.c_str());
	if (ifs.good() == false)
	{
		std::cerr <<  "Probability Analysis ERROR: can not open probability file " << fname << " !" << std::endl;
		exit(-1);
	}
	else
	{
		std::string line1,line2;
		std::getline(ifs, line1);		//read param values
		std::getline(ifs, line2);		//read prob values
		std::stringstream linestream1(line1);
		std::stringstream linestream2(line2);
		while (linestream1.good() && linestream2.good())
		{
			apppack tmp;
			linestream1 >> tmp.appknob;
			linestream2 >> tmp.prob;
			myvec.push_back(tmp);
#ifdef DEBUG            
            cout << "file name: " << fname << ", appknob: " << tmp.appknob << " prob: " << tmp.prob << endl;
#endif
		}
		//myvec.pop_back();			//to be or not to be!?
		return myvec;
	}

}

int findlastidx(std::vector<apppack> &A, double lifetime)		//fist at the right position
{
	int siz = (int)A.size();
	//int finidx = -1;
	for (int i = 0; i < siz; i++)
	{
		if (lifetime < A.at(i).appknob)
			return i;	//finidx = i;
	}
	//return finidx;
}

struct idx_pack     //change
{
	int L1_idx;
	int L2_idx;
	int add_idx;	
    int mul_idx;
	int dram_idx;
};

class res_pack
{
	public:
		long double prob;
		idx_pack idxes;
		res_pack(long double a):prob(a){};
		res_pack(long double a, int b ,int c, int d,int e,int f)
			{prob = a; idxes.L1_idx = b; idxes.L2_idx = c; idxes.add_idx = d; idxes.mul_idx = e; idxes.dram_idx = f;};      //change
};

struct res_pack_cmp {
	bool operator() (const res_pack& lhs, const res_pack& rhs) const
	{
		return lhs.prob<rhs.prob;
	}
};

//std::set<res_pack,res_pack_cmp> final_res;
std::vector<res_pack> final_res;

int main(int argc, const char* argv[])
{
	if(argc < 3)
	{
		std::cerr << "Probability Analysis ERROR: check input arguments! \n USAGE: " << argv[0] << " 0.9 approx.log" << std::endl;
		return -1;
	}
  
	//std::cout << "-- Start of operation --" << std::endl;
	//print_time();
	std::vector<appvar> approx_vars;
	long double Ptarget = std::atof(argv[1]);
	std::string appfile = argv[2];
	std::ifstream ifs(appfile.c_str());
	UInt64 prev_addr = 0;

	//LOADING approx.log file
	char titles[256];
	if (ifs.good())
		ifs.getline(titles,256);		//first line thrown away (redundant getline)
	else
	{
		std::cerr << "Probability Analysis ERROR: can not open approx file!" << std::endl;
		return -1;
	}
	double max_mlifetime(-1),min_mlifetime(DOUBLE_MAX);
	while (ifs.good()) 	
	{
		UInt64 addr;	
		int L1_reads, L1_writes, L2_reads, L2_writes;
		double	mlifetime;
		int add_cnt;
		int mul_cnt;        //change
		//ADDR 		L1_READS		L1_WRITES	L2_READS 		L2_WRITES   ADD_CNT		LIFETIME
		ifs >> addr	 >> L1_reads >> L1_writes >>  L2_reads >> L2_writes  >> mlifetime >> add_cnt >> mul_cnt;	//change

		//setting to appvar
		appvar tmpappvar;
		tmpappvar.addr = addr;
        tmpappvar.L1_reads = L1_reads;
        tmpappvar.L1_writes = L1_writes;
        tmpappvar.L2_reads = L2_reads;
        tmpappvar.L2_writes = L2_writes;
		tmpappvar.maxlifetime = new double(mlifetime*1e-9);		//important !!! ns to s conversion
		tmpappvar.add_cnt = add_cnt;            //change
        tmpappvar.mul_cnt = mul_cnt;
		
		approx_vars.push_back(tmpappvar);	

		if(mlifetime*1e-9 > max_mlifetime)
			max_mlifetime = mlifetime*1e-9;
		if(mlifetime*1e-9 < min_mlifetime)
			min_mlifetime = mlifetime*1e-9;
        
#ifdef  DEBUG
        cout << "Addr: " <<  addr << " " << L1_reads << " " << L1_writes << " " <<  L2_reads << " " << L2_writes << " " << add_cnt << " " << mul_cnt << " " << mlifetime << endl;	//change
#endif        
	}
	//std::cout << "Max and min lifetimes: "  << max_mlifetime << "s \t" << min_mlifetime << std::endl;
	//return 1;

	//loading probability files
	std::vector<apppack> sram_read_L1 	;//= load_probvals("L1_RP_SRAM_6T_45nm.matrix");
	std::vector<apppack> sram_write_L1 	;//= load_probvals("L1_WP_SRAM_6T_45nm.matrix");
	std::vector<apppack> sram_read_L2 	;//= load_probvals("L2_RP_SRAM_6T_45nm.matrix");
	std::vector<apppack> sram_write_L2 	;//= load_probvals("L2_WP_SRAM_6T_45nm.matrix");	
	std::vector<apppack> approx_add;
	std::vector<apppack> approx_mul;        //change
	std::vector<apppack> dram_hold_freq 		;//= load_probvals("DRAM_1T_45nm.matrix");
	std::vector<apppack> dram_hold_time 		;//= load_probvals("DRAM_1T_45nm.matrix");

    std::string exe_dir = dirname(realpath(argv[0], 0));
	sram_read_L1 = 	load_probvals(exe_dir + "/L1_RP_SRAM_6T_45nm.matrix");
	sram_write_L1= 	load_probvals(exe_dir + "/L1_WP_SRAM_6T_45nm.matrix");
	sram_read_L2 = 	load_probvals(exe_dir + "/L2_RP_SRAM_6T_45nm.matrix");
	sram_write_L2= 	load_probvals(exe_dir + "/L2_WP_SRAM_6T_45nm.matrix");
	approx_add = 	load_probvals(exe_dir + "/APPROX_ADD.matrix");
	approx_mul = 	load_probvals(exe_dir + "/APPROX_MUL.matrix");     //change
	dram_hold_freq= load_probvals(exe_dir + "/DRAM_Flikker.matrix");
	
	//change freq to time
	dram_hold_time = dram_hold_freq;
	for (int i=0; i<dram_hold_freq.size();i++)
	{
		dram_hold_time[i].appknob = 1/dram_hold_freq[i].appknob;
	}
	std::cout.precision(15);

	/*
	std::cout << dram_hold_time[0].appknob << "\t\t" << dram_hold_time[0].prob  <<std::endl;
	std::cout << dram_hold_time[1].appknob << "\t\t" << dram_hold_time[1].prob  <<std::endl;
	std::cout << dram_hold_time[2].appknob << "\t\t" << dram_hold_time[2].prob  <<std::endl;
	std::cout << dram_hold_time[3].appknob << "\t\t" << dram_hold_time[3].prob  <<std::endl;
	std::cout << dram_hold_time[4].appknob << "\t\t" << dram_hold_time[4].prob  <<std::endl;
	std::cout << dram_hold_time[5].appknob << "\t\t" << dram_hold_time[5].prob  <<std::endl;
	std::cout << dram_hold_time[6].appknob << "\t\t" << dram_hold_time[6].prob  <<std::endl;
	std::cout << dram_hold_time[7].appknob << "\t\t" << dram_hold_time[7].prob  <<std::endl;
	std::cout << dram_hold_time[8].appknob << "\t\t" << dram_hold_time[8].prob  <<std::endl;
*/

	
	//print_time();
    //return 1;
	//and now exhastive search all memories
	
	std::set<long double> Pmean_rec;
	for (int i = 0; i < (int)sram_read_L1.size(); i++)	//L1
	{	
        std::cout << " ---- loop iteration " << i << " ---- " << std::endl;
        print_time();
        for (int j = 0; j < (int)sram_read_L2.size(); j++)      //L2
        {
			for (int k=0; k< (int) approx_add.size(); k++)		//APPROX_ADD
			{
                for (int l=0; l < (int)approx_mul.size(); l++)      //APPROX_MUL        //change
                {
                    long double Pmean;
                    long double cnter = (double)approx_vars.size();
                    long double P_L1_reads;
                    long double P_L1_writes;
                    long double P_L2_reads;
                    long double P_L2_writes;
                    long double P_add_cnt;
                    long double P_mul_cnt;      //change
                    long double P_dram_hold;
                    long double newP;
                    for (int z = 0; z < (int)dram_hold_time.size(); z++)		//DRAM
                    {
                        if (sram_read_L1.at(i).prob 	< Ptarget || 		//reads are worser than writes
                            sram_read_L2.at(j).prob 	< Ptarget ||
                            approx_add.at(k).prob 		< Ptarget ||
                            approx_mul.at(l).prob 		< Ptarget ||        //change
                            dram_hold_time.at(z).prob 	< Ptarget) 
                            break;							
                        Pmean = 0;
//change
#pragma omp parallel for reduction(+:Pmean) default(none) \
private(newP,P_L1_reads,P_L1_writes,P_L2_reads,P_L2_writes,P_add_cnt,P_mul_cnt,P_dram_hold) \
shared(approx_vars,sram_read_L1,sram_read_L2,sram_write_L1,sram_write_L2,approx_add,approx_mul,dram_hold_time,\
Ptarget,final_res,i,j,k,l,z)					
                        for (int id = 0; id < (int)approx_vars.size(); id++) 
                        {
                    
                            P_L1_reads = powl(sram_read_L1.at(i).prob, 			approx_vars[id].L1_reads);		//^(it->findRcount(0,"L1-D"))
                            P_L1_writes = powl(sram_write_L1.at(i).prob, 		approx_vars[id].L1_writes);
                            P_L2_reads = powl(sram_read_L2.at(j).prob, 			approx_vars[id].L2_reads);
                            P_L2_writes = powl(sram_write_L2.at(j).prob, 		approx_vars[id].L2_writes);
                            P_add_cnt = powl(approx_add.at(k).prob, 			approx_vars[id].add_cnt);
                            P_mul_cnt = powl(approx_mul.at(l).prob,            approx_vars[id].mul_cnt);       //change
                            P_dram_hold = 1;//dram_hold_time.at(z).prob;		//normal (time or freq) values are in ns inside file
        
                            newP = P_L1_reads*P_L1_writes*P_L2_reads*P_L2_writes*P_add_cnt*P_mul_cnt*P_dram_hold;     //change
                            Pmean += newP;		
                            //cout << id << ", " << P_L1_reads << " " << P_L1_writes << " "<< P_L2_reads<< " " << P_L2_writes << " "<< P_add_cnt << " "<< P_dram_hold << " > " << newP << endl;
                        }
                        Pmean = Pmean / cnter;
                        //std::cout << "Pmean value: " << Pmean << std::endl;
                        {
                            if (i == 0 && j == 0 && z == 0)
                            {
                                int AAAA = 1000;//std::cout << "DEBUG" << std::endl;
                                
                            }
                            //	std::cout << "thread num: " << omp_get_thread_num() << " Pmean, Pfinal, z " << Pmean << " " << Pfinal << " " << z  << std::endl;
                            if (Pmean > Ptarget)    //new result		last_Pmean != Pmean
                            {	
                                res_pack tmp(Pmean,i,j,k,l,z);      //change
                                final_res.push_back(tmp);
                                //cout << "New results idx: " << " L1: " << i << " L2: " << j << " ADD: " << k << " DRAM: " << z << endl;
                                //std::pair<std::set<res_pack>::iterator,bool> ret = final_res.insert(tmp);
                                //if(ret.second == false)	//if not inserted -> updating
                                //{
                                //	if(ret.first->idxes.dram_idx > z)
                                //		tmp.idxes.dram_idx = ret.first->idxes.dram_idx;
                                //	final_res.erase(ret.first);
                                //	final_res.insert(tmp);
                                //}
                    
                            }
                        }
                    }
                }
            }
		}
	}
	std::cout << "-- End of operation --" << std::endl;
	//print_time();

	//std::cout << "-- Results --" << std::endl;
	if (final_res.size() == 0)
	{
		std::cout << "Probability Analysis WARNING: No results available." << std::endl;
		// writing error-free values.
		std::ofstream paramfile("Candidate_appknobValues.txt");
		paramfile << 1 << std::endl;        //number of candidate results
		paramfile << "l1: " << sram_read_L1.at(0).appknob 	<< std::endl;
		paramfile << "l2: "  <<  sram_read_L2.at(0).appknob 	<< std::endl;
		paramfile << "add: " << approx_add.at(0).appknob   	<< std::endl;
		paramfile << "mul: " << approx_mul.at(0).appknob   	<< std::endl;       //change
		if(max_mlifetime < 0.064)
			paramfile << "dram: " << 15.625	<< std::endl;		
		else		
			paramfile << "dram: " <<  dram_hold_freq.at(0).appknob 		<< std::endl;		
		std::ofstream faultfile("Candidate_faultProbs.txt");
		faultfile << 1 << std::endl;		
		faultfile << "l1\t" << 1 - sram_read_L1.at(0).prob << "\t0\t" << 1 - sram_write_L1.at(0).prob << std::endl;
		faultfile << "l2\t" << 1 - sram_read_L2.at(0).prob << "\t0\t" << 1 - sram_write_L2.at(0).prob << std::endl;
		faultfile << "add\t" << 1 - approx_add.at(0).prob << "\t0\t" << 1 - approx_add.at(0).prob << std::endl;
		faultfile << "mul\t" << 1 - approx_mul.at(0).prob << "\t0\t" << 1 - approx_mul.at(0).prob << std::endl; //change
		faultfile << "dram\t0\t" << 1 - dram_hold_freq.at(0).prob << "\t0" << std::endl;
		return 0;
	}
	else
	{
		std::ofstream paramfile("Candidate_appknobValues.txt");
		paramfile << final_res.size() << std::endl;
		//std::set<res_pack, res_pack_cmp>::iterator it;
		std::vector<res_pack>::iterator it;
		for (it = final_res.begin(); it != final_res.end(); ++it)
		{
			paramfile << "l1: " << sram_read_L1.at(it->idxes.L1_idx).appknob 	<< std::endl;
			paramfile << "l2: "  << sram_read_L2.at(it->idxes.L2_idx).appknob 	<< std::endl;
			paramfile << "add: "  << approx_add.at(it->idxes.add_idx).appknob 	<< std::endl;
			paramfile << "mul: "  << approx_mul.at(it->idxes.mul_idx).appknob 	<< std::endl;       //change
			paramfile << "dram: "<< dram_hold_freq.at(it->idxes.dram_idx).appknob 	<< std::endl;			
		}
		
		std::ofstream faultfile("Candidate_faultProbs.txt");
		faultfile << final_res.size() << std::endl;
		for (it = final_res.begin(); it != final_res.end(); ++it)
		{
			faultfile << "0\tl1\t" << 1 - sram_read_L1.at(it->idxes.L1_idx).prob << "\t0\t" << 1 - sram_write_L1.at(it->idxes.L1_idx).prob << std::endl;
			faultfile << "0\tl2\t" << 1 - sram_read_L2.at(it->idxes.L2_idx).prob << "\t0\t" << 1 - sram_write_L2.at(it->idxes.L2_idx).prob << std::endl;
			faultfile << "0\tadd\t" << 1 - approx_add.at(it->idxes.add_idx).prob << "\t0\t" << std::endl;
			faultfile << "0\tmul\t" << 1 - approx_mul.at(it->idxes.mul_idx).prob << "\t0\t" << std::endl;       //change
			faultfile << "0\tdram\t0\t" << 1 - dram_hold_freq.at(it->idxes.dram_idx).prob << "\t0" << std::endl;
		}		

	}
    
	//std::cin.get();
	return 0;
}

/*
					if (Pmean <= Pfinal && Pmean >= Ptarget)
					{
						Pfinal = Pmean;
						ifin = i; jfin = j; 
						//just to handle parallel :)
						if (Pmean < Pfinal)
							zfin = z;
						else if (Pmean == Pfinal && z > zfin)
							zfin = z;
					}

*/