#include "world.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>
#include <iomanip>
#include "timing.h"
#include "graph.h"

int main(int argc, const char** argv)
{
    char* inputFile = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0)
            *inputFile = argv[i+1]
    }

    if (inputFile == NULL)
        std::cout << "Error: must specify input file with -i\n";
    return 1;

    Graph g = loadGraph(inputFile); // function doesn't exist yet


    // run, time, and check correctness
    Timer t;
    MST res = Boruvka(g);
    double totalTime = t.elapsed();

    if (checkCorrectness(res, g)) {
        printf("MST computed successfully!\n");
    } else {
        printf("Correctness failed.");
    }

    printf("Total time: %.6fms", totalTime);

    return 0;
}
