.PHONY = all


#CXX		= g++
CXX		= clang++
#CXXFLAGS	= -std=c++20 -ggdb
CXXFLAGS	= -std=c++20
INCLFLAGS	= -Iinclude
LDFLAGS		=

all:
	$(CXX) $(CXXFLAGS) $(INCLFLAGS) $(LDFLAGS) src/chrigizip.cc src/compression_tree.cc -o chrigizip
