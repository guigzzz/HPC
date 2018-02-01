SHELL=/bin/bash

CPPFLAGS += -std=c++11 -W -Wall  -g
CPPFLAGS += -O3
CPPFLAGS += -I include

LDLIBS += -ltbb

ifeq ($(OS),Windows_NT)
LDLIBS += -lws2_32
LDLIBS += -lOpenCL
else ifeq ($(shell uname 2>/dev/null),Darwin)
# lrt not needed on macOS
LDLIBS += -framework OpenCL
else
LDLIBS += -lrt
LDLIBS += -lOpenCL
endif

all : bin/execute_puzzle bin/create_puzzle_input bin/run_puzzle bin/compare_puzzle_output bin/run_puzzle_custom

lib/libpuzzler.a : $(wildcard provider/*.cpp provider/*.hpp include/puzzler/*.hpp include/puzzler/*/*.hpp)
	cd provider && $(MAKE) all

bin/% : src/%.cpp lib/libpuzzler.a
	-mkdir -p bin
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) -Llib -lpuzzler


serenity_now_% : all
	mkdir -p w
	bin/run_puzzle $* 100 2
	bin/create_puzzle_input $* 100 2 > w/$*.in
	cat w/$*.in | bin/execute_puzzle 1 2 > w/$*.ref.out
	cat w/$*.in | bin/execute_puzzle 0 2 > w/$*.got.out
	bin/compare_puzzle_output w/$*.in w/$*.ref.out w/$*.got.out 2

serenity_now : $(foreach x,mining heat_world gaussian_blur edit_distance random_projection hold_time,serenity_now_$(x))

test: all
	@echo "Testing correctness"
	scripts/test_correctness
