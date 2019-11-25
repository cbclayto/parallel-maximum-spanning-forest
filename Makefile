CFLAGS := -std=c++11 -fvisibility=hidden -O3
SOURCES := src/main.cpp src/graph.cpp src/boruvka.cpp src/prim.cpp

msf-solver: src/*.cpp
	g++ -o msf-solver $(CFLAGS) $(SOURCES)
