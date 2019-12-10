#include <string>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include <omp.h>

#include "parallel_prim_blocks.h"

int find_nearest_blocks(int *dists, int offset, int dist_len){
   int nearest = -1;
   int nearest_dist = INT_MAX;
   for (int i = 0; i < dist_len; i++){
       if (dists[offset + i] > INT_MIN && dists[offset + i] < nearest_dist) {
           nearest = i;
           nearest_dist = dists[offset + i];
       }
   }
   return nearest;
}

inline void merge_all(int *dists, int *colors, int *merge_me_with, int num_nodes, int num_threads)
//merges everything 
{
    for (int thread = 1; thread < num_threads; thread++){
            int offset_other = num_nodes * thread;
            for (int node = 0; node < num_nodes; node++){
                dists[node] = std::min(dists[node], dists[offset_other + node]);
                if (thread == num_threads - 1 && dists[node] == INT_MIN){
                    colors[node] = 0;
                }
            }
    }
}


int parallel_prims_blocks(std::shared_ptr<Graph> graph){
    int num_nodes = graph->num_nodes;
    int respawn_cutoff = num_nodes/2; // PLACE TO OPTIMIZE
    std::vector<std::vector<std::shared_ptr<Edge>>> edges = graph->edges;
    int num_threads, thread_id;
    int *dists;
    int *colors = (int*)malloc(num_nodes * sizeof(int));
    #pragma omp parallel for
    for (int i = 0; i < num_nodes; i++){
       colors[i] = -1;
    }
    omp_lock_t color_lock;
    omp_init_lock(&color_lock);
    omp_lock_t total_lock;
    omp_init_lock(&total_lock);
    int total_cost = 0;
    omp_lock_t visited_lock;
    omp_init_lock(&visited_lock);
    int total_visited = 0;
    bool *ready_to_reset;
    int *merge_me_with;
    bool done = false;
    #pragma omp parallel private(thread_id) num_threads(16) // PLACE TO OPTIMIZE
    {
       thread_id = omp_get_thread_num();
       if (thread_id == 0){
          num_threads = omp_get_num_threads();
          dists = (int*)malloc(num_threads * num_nodes * sizeof(int));
          ready_to_reset = (int*)malloc(num_threads * sizeof(bool));
          merge_me_with = (int*)malloc(num_threads * sizeof(int));
       }
       #pragma omp barrier
       int offset = num_nodes * thread_id;

       int my_cost;
       int start_node;

       if (thread_id == 0){
           for (int i = 0; i < num_nodes; i++){
                   dists[offset + i] = INT_MAX;
               }
               my_cost = 0;
               start_node = 0;
               colors[start_node] = 0;

               //Add start_node right away
               dists[offset + start_node] = INT_MIN;
               for (int i = 0; i < edges[start_node].size(); i++){
                   dists[offset + (edges[start_node][i]->endpoint)] = edges[start_node][i]->weight;
               }
       }

       while (!done){        
            
           if (ready_to_reset[thread_id] && num_nodes - total_visited >= respawn_cutoff) {
               //reset!
               for (int i = 0; i < num_nodes; i++){
                   dists[offset + i] = INT_MAX;
               }
               my_cost = 0;
               start_node = rand() % num_nodes;
               int attempts = 0;
               omp_set_lock(&color_lock);
               while (colors[start_node] != -1){
                   start_node = rand() % num_nodes;
               }
               colors[start_node] = thread_id;
               omp_unset_lock(&color_lock);
               //Add start_node right away
               dists[offset + start_node] = INT_MIN;
               for (int i = 0; i < edges[start_node].size(); i++){
                   dists[offset + (edges[start_node][i]->endpoint)] = edges[start_node][i]->weight;
               }
           }


           #pragma omp barrier

            while (thread_id == 0 || num_nodes - total_visited >= respawn_cutoff) {               int nearest = find_nearest_blocks(dists, offset, num_nodes);

               //if there are no more nodes
               if (nearest == -1) {
                   omp_set_lock(&total_lock);
                   total_cost += my_cost;
                   omp_unset_lock(&total_lock);
                   omp_set_lock(&color_lock);
                   merge_all(dists, colors, num_nodes, num_threads);
                   omp_unset_lock(&color_lock);
                   done = 1;
                   break;
               }

               my_cost += dists[offset + nearest];
               
               omp_set_lock(&color_lock);
               if (colors[nearest] == -1){
                 colors[nearest] = thread_id;
                 omp_unset_lock(&color_lock);
                 omp_set_lock(&visited_lock);
                 total_visited += 1;
                 omp_unset_lock(&visited_lock);
               } else {
                 merge_me_with[thread_id] = colors[nearest];
                 omp_unset_lock(&color_lock);
                 break;
               }

               //This is the case where we can proceed
               dists[offset + nearest] = INT_MIN;
               for (int j = 0; j < edges[nearest].size(); j++){
                   int vertex = edges[nearest][j]->endpoint;
                   int weight = edges[nearest][j]->weight;
                   dists[offset + vertex] = std::min(weight, dists[offset + vertex]);
                }
            }
            #pragma omp barrier
            if (thread_id == 0){
               merge_all(dists, colors, merge_me_with, num_nodes, num_threads);
            }
        }
    }
    omp_destroy_lock(&color_lock);
    omp_destroy_lock(&total_lock);
    omp_destroy_lock(&visited_lock);
    return total_cost;
}
