#include <string>
#include <cstdlib>
#include <limits.h>
#include <omp.h>

#include "prim.h"

int find_nearest(int *dists, int offset, int dist_len){
   int nearest = 0;
   int nearest_dist = INT_MAX;
   for (int i = 0; i < dist_len; i++){
       if (dists[offset + i] > INT_MIN && dists[offset + i] < nearest_dist) {
           nearest = i;
           nearest_dist = dists[offset + i];
       }
   }
   return nearest;
}

void merge(int *dists, int other_thread, int thread_id, int num_nodes){
   int offset_other = num_nodes * other_thread;
   int offset_this = num_nodes * thread_id;
   for (int i = 0; i < num_nodes; i++){
     dists[offset_other + i] = std::min(dists[offset_other + 1], dists[offset_this + 1]);
   }
} 


int prims(std::shared_ptr<Graph> graph){
    int num_nodes = graph->num_nodes;
    std::vector<std::vector<std::shared_ptr<Edge>>> edges = graph->edges;
    int num_threads, thread_id;
    int *dists;
    omp_lock_t *locks;
    omp_lock_t total_lock;
    omp_init_lock(&total_lock);
    int total_cost = 0;
    #pragma omp parallel private(thread_id) num_threads(2)
    {
       thread_id = omp_get_thread_num();
       if (thread_id == 0){
          num_threads = omp_get_num_threads();
          dists = (int*)malloc(num_threads * num_nodes * sizeof(int));
          locks = (omp_lock_t *)malloc(num_threads * sizeof(omp_lock_t));
          printf("There are %d threads. \n", num_threads);
       }
       #pragma omp barrier
       omp_init_lock(&locks[thread_id]);
       int offset = num_nodes * thread_id;
       for (int i = 0; i < num_nodes; i++){
           dists[offset + i] = INT_MAX;
       }
       int my_cost = 0;
       int start_node = thread_id;
       if (start_node < num_nodes){
           //Add start_node right away
           dists[offset + start_node] = INT_MIN;
           for (int i = 0; i < edges[start_node].size(); i++){
               dists[offset + (edges[start_node][i]->endpoint)] = edges[start_node][i]->weight;
           }
       }
       #pragma omp barrier
       if (start_node < num_nodes) {
           //add more nodes
           while (true) {
               omp_set_lock(&locks[thread_id]);
               int nearest = find_nearest(dists, offset, num_nodes);
               int other_thread = -1;
               for (int k = 0; k < num_threads; k++){
                  if (k != thread_id && dists[(num_nodes * k) + nearest] == INT_MIN){
                      other_thread = k;
                  }
               }
               //if we found a node another thread already found
               if (other_thread != -1){
                   omp_set_lock(&locks[other_thread]);
                   merge(dists, other_thread, thread_id, num_nodes); //RACE!
                   omp_unset_lock(&locks[other_thread]);
                   my_cost += dists[offset + nearest];
                   omp_unset_lock(&locks[thread_id]);
                   printf("Bye!\n");
                   break;
               }
               //if there are no more nodes
               if (dists[offset + nearest] == INT_MIN ||
                   dists[offset + nearest] == INT_MAX){
                   omp_set_lock(&total_lock);
                   total_cost += my_cost; //LOCK
                   printf("Thread %d here. The cost is %d.\n", thread_id, total_cost);
                   omp_unset_lock(&total_lock);
                   omp_unset_lock(&locks[thread_id]);
                   break;
               }
               my_cost += dists[offset + nearest];
               dists[offset + nearest] = INT_MIN;
               for (int j = 0; j < edges[nearest].size(); j++){
                   int vertex = edges[nearest][j]->endpoint;
                   int weight = edges[nearest][j]->weight;
                   dists[offset + vertex] = std::min(weight, dists[offset + vertex]);
                }
                omp_unset_lock(&locks[thread_id]);
            }
       }
       #pragma omp barrier
       omp_destroy_lock(&locks[thread_id]);
    }
    omp_destroy_lock(&total_lock);
    return total_cost;
}
