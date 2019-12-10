#include <cstdlib>
#include <iostream>
#include <string.h>
#include "timing.h"
#include "graph.h"
#include "boruvka.h"
#include "prim.h"
#include "parallel_boruvka.h"
#include "parallel_prim.h"
#include "merge_boruvka.h"
#include "boruvka_FAL.h"
#include "simple_parallel_prim.h"

void run_prim(std::shared_ptr<Graph> G) {
    std::cout << "Prim's:\n";

    Timer t;
    int weight = prims(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

void run_pprim(std::shared_ptr<Graph> G) {
    std::cout << "Parallel Prim's:\n";

    Timer t;
    int weight = parallel_prims(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

void run_sprim(std::shared_ptr<Graph> G) {
    std::cout << "Simple Parallel Prim's:\n";

    Timer t;
    int weight = simple_parallel_prims(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

void run_boruvka(std::shared_ptr<Graph> G) {
    std::cout << "Boruvka's:\n";

    Timer t;
    int weight = boruvka(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

void run_pboruvka(std::shared_ptr<Graph> G) {
    std::cout << "Parallel Boruvka's:\n";

    Timer t;
    int weight = parallel_boruvka(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

void run_mergeboruvka(std::shared_ptr<Graph> G) {
    std::cout << "Merging Boruvka's:\n";

    int weight = merging_boruvka(G);
}

void run_pmergeboruvka(std::shared_ptr<Graph> G) {
    std::cout << "Parallel Merging Boruvka's:\n";

    int weight = parallel_merging_boruvka(G);
}

void run_FALboruvka(std::shared_ptr<FAL_Graph> G) {
    std::cout << "FAL Boruvka's:\n";

    Timer t;
    int weight = FAL_boruvka(G);
    double time = t.elapsed();
    std::cout << "\tMST weight: " << weight << "\n";
    printf("\tTime: %.6fms\n\n", time);
}

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

    std::shared_ptr<FAL_Graph> G_fal = load_FAL_graph(inputFile);
    std::shared_ptr<Graph> G = load_graph(inputFile);
    std::shared_ptr<Graph> G_ = load_graph(inputFile);

    //print_FAL_graph(G_fal, false);
    //print_graph(G, false);

    // run, time, and check correctness

    run_prim(G);
    run_pprim(G);
    run_sprim(G);
    run_boruvka(G);
    run_pboruvka(G);
    run_mergeboruvka(G);
    run_pmergeboruvka(G_);
    run_FALboruvka(G_fal);

    return 0;
}
