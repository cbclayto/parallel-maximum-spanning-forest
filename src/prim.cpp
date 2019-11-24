#include <string>
#include <cstdlib>

#include "graph.h"

int find_nearest(int *dists, int dist_len){
   int nearest = 0;
   int nearest_dist = INT_MAX;
   for (int i = 0; i < dist_len; i++){
       if (dists[i] != INT_MIN && dists[i] < nearest_dist) {
           nearest = i;
           nearest_dist = dists[i];
       }
   }
}

int prims(Graph *graph){
    int num_nodes = graph->num_nodes;
    int *dists = malloc(num_nodes * sizeof(int));
    for (int i = 0; i < num_nodes; i++){
       dists[i] = INT_MAX;
    }
    int total_cost = 0;
    //Add node 0 right away
    dists[0] = INT_MIN;
    for (int i = 0; i < edges[0].size(); i++){
        dists[edges[0][i].endpoint] = edges[0][i].weight;
    } 
    //add n-1 more nodes
    for (int i = 1; i < num_nodes; i++) {
        int nearest = find_nearest(dists, num_nodes);
        total_cost += dists[nearest];
        dists[nearest] = INT_MIN;
        for (int j = 0; j < edges[nearest].size(); j++){
            int vertex = edges[nearest][j].endpoint;
            int weight = edges[nearest][j].weight;
            dists[vertex] = std::min(weight, dists[vertex]);
        }
    }
}
