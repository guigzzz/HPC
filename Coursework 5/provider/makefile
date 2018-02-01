SHELL=/bin/bash

CPPFLAGS += -std=c++11 -W -Wall  -g
CPPFLAGS += -O3

# Suppress included warnings
CPPFLAGS += -isystem ../include

# Force IEEE. As suggested in issue.
CPPFLAGS += -msse2 -ffloat-store

puzzles.o : $(wildcard *.hpp) $(wildcard ../include/puzzler/*.hpp ../include/puzzler/*/*.hpp)

../lib/libpuzzler.a : puzzles.o
	-mkdir -p ../lib
	ar cr ../lib/libpuzzler.a puzzles.o

all : ../lib/libpuzzler.a
