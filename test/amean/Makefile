TARGET=amean
include ../shared/Makefile.shared

$(TARGET): *.cpp
	$(SNIPER_CXX) *.cpp -lm $(SNIPER_LDFLAGS) -I${SNIPER_ROOT}/include -std=c++11 -D WIDHEI=16  -D SNIPER -O0  -o $(TARGET)

run_$(TARGET): 
	../../run-sniper -n 1 -c raul --roi  --no-cache-warming  -g --perf_model/cache/levels=2 --cache-only -g --amean/simple/cores_per_socket=1 -- ./amean 

#show:
#	head approx.log

