# Heap memory check worked
> env HEAPCHECK=normal $PROJ_HOME/bin/prof_test_d 10
# daemon output shows how leaks can be identified using google-pprof command

# Heap profiling worked
> env HEAPPROFILE=prof_test.hprof prof_test 10
> google-pprof --gv $PROJ_HOME/bin/prof_test_d prof_test.hprof.0001.heap
# displays graphically the heap memory allocation profile 

# CPU Profiling was not enabled when we enabled GPerfTools:CPUProfiler:
> env CPUPROFILE=prof_test.cprof $PROJ_HOME/bin/prof_test_d 10

# The reason was diagnosed to profiler not linking to the executable:
> ldd $PROJ_HOME/bin/prof_test_d
# Only displays libtcmalloc.so but not libprofiler.so

# LINKER ISSUE: The issue boiled down to bug in linker
# Note that ld library paths were fine as libprofiler.so 
# was in the correct directory and ldconfig -p showed the 
# library was visible
# When profiler library was preloaded by linker the profiler worked: 
> env LD_PRELOAD=/usr/lib/libprofiler.so env CPUPROFILE=prof_test.cprof $PROJ_HOME/bin/prof_test_d 10

# Also identified specific options to linker that forced profiler to be loaded
# This option worked as well:
> g++ -std=c++11 -Wall -Werror -g -o $PROJ_HOME/bin/prof_test_d prof_test.cc -Wl,--no-as-needed -lprofiler -Wl,--as-needed
> env CPUPROFILE=prof_test.cprof $PROJ_HOME/bin/prof_test_d 10
