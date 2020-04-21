TARGET=sobel
include ../shared/Makefile.shared

$(TARGET): *.cpp
	$(SNIPER_CXX) *.cpp -I${SNIPER_ROOT}/include -std=c++11 -D IMHEADER='"/home/shahdaad/raul/images/hpp/peppers_gray.hpp"' -D WIDHEI=512 -D SNIPER -O0  -o $(TARGET)

run_$(TARGET): 
	../../run-sniper -n 1 -c raul --roi --no-cache-warming  -g --perf_model/cache/levels=2 --cache-only  -g --sobel/simple/cores_per_socket=1 -- ./sobel 

show:
	head approx.log

