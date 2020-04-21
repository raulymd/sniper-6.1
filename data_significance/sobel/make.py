
import os,sys

if (len(sys.argv) < 3):
    print('ERROR: check input arguments. \n USAGE: python make.py IMNAME IMSIZE')
    exit(-1)

bname = "sobel"
imname = str(sys.argv[1])
imsize = str(sys.argv[2])
if(len(sys.argv) > 3):
	fault_prob = float(sys.argv[3])
else:
	fault_prob = 0
    
part_1 = 'g++ *.cpp -std=c++11 -o ' + bname + '_' + imname + '_' + imsize
part_2 = ' -D IMHEADER=\\' + '"/home/shahdaad/raul/images/hpp/'  + imname + '_gray.hpp\\" '
part_3 = ' -D WIDHEI=' + imsize
if(len(sys.argv) > 3):
	part_4 = ' -D DATA_SIG	-D FAULT_PROB=' + str(fault_prob)
else:
	part_4 = ''

string  = part_1 + part_2 + part_3 + part_4

#print("NOTE: using grayscale images as input")
print (string)
assert os.system(string)==0
