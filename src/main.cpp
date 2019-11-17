#include <cstdlib>
#include <iostream>
#include <string.h>
#include "timing.h"
#include "graph.h"

int main(int argc, const char** argv)
{
    const char* inputFile = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0)
            inputFile = argv[i+1];
    }

    if (inputFile == NULL) {
        std::cout << "Error: must specify input file with -i\n";
        return 1;
    }

    //std::shared_ptr<FAL_Graph> G_fal = load_FAL_graph(inputFile);
    std::shared_ptr<Graph> G = load_graph(inputFile);

    //print_FAL_graph(G_fal, false);
    print_graph(G, false);

    // run, time, and check correctness
    /*
    Timer t;
    MST res = Boruvka(G);
    double totalTime = t.elapsed();

    if (checkCorrectness(res, G)) {
        std::cout << "MST computed successfully!\n";
    } else {
        std::cout << "Correctness failed.";
    }

    printf("Total time: %.6fms", totalTime);
    */

    return 0;
}
