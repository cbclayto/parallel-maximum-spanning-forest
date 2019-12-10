#include <string>
#include <cstdlib>
#include <limits.h>
#include <omp.h>

#include "simple_prim.h"

int find_nearest_simple(int *dists, int dist_len){
   int nearest = 0;
   int nearest_dist = INT_MAX;
   for (int i = 0; i < dist_len; i++){
       if (dists[i] > INT_MIN && dists[i] < nearest_dist) {
           nearest = i;
           nearest_dist = dists[i];
       }
   }
   return nearest;
}

int simple_parallel_prims(std::shared_ptr<Graph> graph){
    int num_nodes = graph->num_nodes;
    int *dists = (int*)malloc(num_nodes * sizeof(int));
    #pragma omp parallel for
    for (int i = 0; i < num_nodes; i++){
       dists[i] = INT_MAX;
    }
    int total_cost = 0;
    int start_node = 0;
    //Add start_node right away
    dists[start_node] = INT_MIN;

    std::vector<std::vector<std::shared_ptr<Edge>>> edges = graph->edges;
    #pragma omp parallel for
    for (int i = 0; i < edges[start_node].size(); i++){
        dists[edges[start_node][i]->endpoint] = edges[start_node][i]->weight;
    } 
    //add n-1 more nodes
    while (true) {
        int nearest = find_nearest_simple(dists, num_nodes);
        if (dists[nearest] == INT_MIN ||
            dists[nearest] == INT_MAX){
            return total_cost;
        }
        total_cost += dists[nearest];
        dists[nearest] = INT_MIN;
        #pragma omp parallel for
        for (int j = 0; j < edges[nearest].size(); j++){
            int vertex = edges[nearest][j]->endpoint;
            int weight = edges[nearest][j]->weight;
            dists[vertex] = std::min(weight, dists[vertex]);
        }
    }
}
