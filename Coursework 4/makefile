CXXFLAGS += -std=c++11
CPPFLAGS += -g
CPPFLAGS += -O3
CPPFLAGS += -I include

#CPPFLAGS += -I opencl_sdk/include
#LDFLAGS += -L opencl_sdk/lib/cygwin/x86
#LDFLAGS += -L opencl_sdk/lib/windows/x86_64

LDLIBS += -ltbb
#LDLIBS += -lOpenCL

# Roll on better platform support...
CPPFLAGS += -Wno-deprecated-declarations

DEFAULT_SRCS = src/util/layer_io.cpp
LAYER_SRCS = $(wildcard src/layers/*.cpp)
TOOL_SRCS = $(wildcard src/tools/*.cpp)

bin/% : src/tools/%.cpp $(DEFAULT_SRCS)
	mkdir -p bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	
bin/run_network : src/tools/run_network.cpp $(DEFAULT_SRCS) $(LAYER_SRCS)
	mkdir -p bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)


tools : $(patsubst src/tools/%.cpp,bin/%,$(TOOL_SRCS))
