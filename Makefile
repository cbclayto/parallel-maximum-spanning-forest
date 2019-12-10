CFLAGS := -std=c++11 -fvisibility=hidden -O3 -fopenmp
SOURCES := src/main.cpp src/graph.cpp src/prim.cpp src/boruvka.cpp src/parallel_prim.cpp src/parallel_boruvka.cpp src/merge_boruvka.cpp src/boruvka_FAL.cpp

msf-solver: src/*.cpp
	g++ -o msf-solver $(CFLAGS) $(SOURCES)
