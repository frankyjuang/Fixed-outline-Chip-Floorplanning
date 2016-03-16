CXX=g++
CFLAGS=-g -std=c++11
CFLAGS=-O3 -std=c++11

all:
	$(CXX) focf.cpp -o focf $(CFLAGS)
