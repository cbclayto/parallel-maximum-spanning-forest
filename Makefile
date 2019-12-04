msf-solver: src/*.cpp
	g++ -o msf-solver -std=c++11 -fvisibility=hidden -O3 -fopenmp src/main.cpp src/graph.cpp src/boruvka.cpp src/parallel_prim.cpp
