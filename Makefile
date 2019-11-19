msf-solver: src/main.cpp src/graph.cpp src/boruvka.cpp
	g++ -o msf-solver -std=c++11 -fvisibility=hidden -O3 src/main.cpp src/graph.cpp src/boruvka.cpp
