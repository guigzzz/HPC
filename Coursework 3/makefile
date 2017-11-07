CPPFLAGS += -I include
CPPFLAGS += -W -Wall
CPPFLAGS += -std=c++11
CPPFLAGS += -O3

LDFLAGS +=  -Iopencl_sdk/include -Lopencl_sdk/lib/windows/x86_64
LDLIBS += -lOpenCL -ltbb

all : bin/make_world \
	  bin/render_world \
	  bin/step_world \
	  bin/test_opencl \
	  bin/gr1714/step_world_tbb \
	  bin/gr1714/step_world_v1_lambda \
	  bin/gr1714/step_world_v2_function \
	  bin/gr1714/step_world_v3_opencl \
	  bin/gr1714/step_world_v4_double_buffered \
	  bin/gr1714/step_world_v5_packed_properties

bin/% : src/%.cpp src/heat.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean : 
	rm -r bin