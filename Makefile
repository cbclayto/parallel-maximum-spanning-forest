CFLAGS := -std=c++11 -fvisibility=hidden -O3 -fopenmp
SOURCES := src/main.cpp src/graph.cpp src/boruvka.cpp src/prim.cpp src/parallel_boruvka.cpp

msf-solver: src/*.cpp
	g++ -o msf-solver $(CFLAGS) $(SOURCES)
