""" 
	Developed by: Raul Yarmand
	loop for calculating data significance error probability
	NOTE: script must be executed in testbench folder 
"""

import sys,os,timeit

def datasig(imname,QM_min):
	#print "Running Raul loop..."
	#if(sys.argc < 2):
	#	print "USAGE: Raul-datasig <benchmark>"
	#	sys.exit(-1)
	
	#sys.exit(-1)
	#os.system("")
	bname = os.path.basename(os.getcwd())
	# compiling the original file
	assert os.system("python make.py %s  512" %(imname))==0
	
	# running original file
	assert os.system("./%s_%s_512 golden_%s_512" %(bname,imname,imname))==0
	
	for i in [float(x) / 10000.0 for x in range(0, 100, 1)]:
		print "-----------------------------------------------"
		print "Running loop for FAULT_PROB = %f ..." %(i)
		assert os.system("python make.py %s  512 %f" %(imname,i))==0
		assert os.system("./%s_%s_512 faulty_%s_512" %(bname,imname,imname))==0
		# with assertion make error
		assert os.system("python ../../../utility/psnr.py %s_outimg_golden_%s_512.ch %s_outimg_faulty_%s_512.ch" %(bname,imname,bname,imname))==0
		f = open("return.txt", "r")  
		PSNR = float(f.read())
		f.close()
		if(PSNR < QM_min ):
			print "Loop breaked!"
			break
	return i
	
if __name__ == '__main__':
	min_quality = 35
	ft_vec = []
	#-----------------------------
	retval = datasig("airplain",min_quality)
	ft_vec.append(retval)
	print "airplain: " + str(retval)	
	#-----------------------------
	retval = datasig("baboon",min_quality)
	ft_vec.append(retval)
	print "baboon: " + str(retval)	
	#-----------------------------
	retval = datasig("fruits",min_quality)
	ft_vec.append(retval)
	print "fruits: " + str(retval)	
	#-----------------------------
	retval = datasig("house",min_quality)
	ft_vec.append(retval)
	print "house: " + str(retval)	
	#-----------------------------
	retval = datasig("siosepol",min_quality)
	ft_vec.append(retval)
	print "siosepol: " + str(retval)	
	#-----------------------------
	retval = datasig("splash",min_quality)
	ft_vec.append(retval)
	print "splash: " + str(retval)	
	#-----------------------------
	retval = datasig("truck",min_quality)
	ft_vec.append(retval)
	print "truck: " + str(retval)	
	#-----------------------------
	retval = datasig("peppers",min_quality)
	ft_vec.append(retval)
	print "peppers: " + str(retval)	
	#-----------------------------
	print ft_vec
	print "average: " + str(sum(ft_vec)/len(ft_vec))