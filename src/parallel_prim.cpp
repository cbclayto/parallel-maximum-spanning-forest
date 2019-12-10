#include <string>
#include <cstdlib>
#include <limits.h>
#include <omp.h>

#include "parallel_prim.h"

int find_nearest(int *dists, int offset, int dist_len){
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

void merge(int *dists, int other_thread, int thread_id, int num_nodes){
   int offset_other = num_nodes * other_thread;
   int offset_this = num_nodes * thread_id;
   for (int i = 0; i < num_nodes; i++){
     dists[offset_other + i] = std::min(dists[offset_other + i], dists[offset_this + i]);
   }
} 

void merge_all(int *dists, int *colors, bool *merge_waits, int thread_id, int num_nodes, int num_threads){
    int offset_this = num_nodes * thread_id;
    for (int thread = 0; thread < num_threads; thread++){
        if (thread != thread_id && merge_waits[(num_threads * thread_id) + thread]){
            int offset_other = num_nodes * thread;
            //merge the other thread into me
            for (int i = 0; i < num_nodes; i++){
                dists[offset_this + i] = 
                    std::min(dists[offset_this + i], dists[offset_other + i]);
                if (dists[offset_other + i] == INT_MIN){
                    colors[i] = thread_id;
                }
            }
            //let the other thread go
            //printf("Thread %d just merged into thread %d.\n", thread, thread_id);
            merge_waits[(num_threads * thread_id) + thread] = 0;
        }
    }
}


int parallel_prims(std::shared_ptr<Graph> graph){
    int num_nodes = graph->num_nodes;
    std::vector<std::vector<std::shared_ptr<Edge>>> edges = graph->edges;
    int num_threads, thread_id;
    int *dists;
    int *colors = (int*)malloc(num_nodes * sizeof(int));
    #pragma omp parallel for
    for (int i = 0; i < num_nodes; i++){
       colors[i] = -1;
    }
    omp_lock_t *locks;
    omp_lock_t color_lock;
    omp_init_lock(&color_lock);
    bool* merge_waits;
    omp_lock_t total_lock;
    omp_init_lock(&total_lock);
    int total_cost = 0;
    #pragma omp parallel private(thread_id)
    {
       thread_id = omp_get_thread_num();
       if (thread_id == 0){
          num_threads = omp_get_num_threads();
          dists = (int*)malloc(num_threads * num_nodes * sizeof(int));
          locks = (omp_lock_t *)malloc(num_threads * sizeof(omp_lock_t));
          merge_waits = (bool*)calloc(num_threads * num_threads, sizeof(bool));
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
           colors[start_node] = thread_id;
       }

       #pragma omp barrier
       if (start_node < num_nodes) {
           //add more nodes
           while (true) {
               omp_set_lock(&color_lock);
               omp_set_lock(&locks[thread_id]);
               merge_all(dists, colors, merge_waits, thread_id, num_nodes, num_threads);
               omp_unset_lock(&locks[thread_id]);
               omp_unset_lock(&color_lock);
  
               int nearest = find_nearest(dists, offset, num_nodes);
               //printf("Thread %d's nearest neighbor is %d and the color of 10 is %d.\n", thread_id, nearest, colors[10]);
              
               //if there are no more nodes
               if (nearest == -1) {
                   omp_set_lock(&total_lock);
                   total_cost += my_cost; //LOCK
                   omp_unset_lock(&total_lock);
                   break;
               }

               omp_set_lock(&color_lock);

               if (colors[nearest] == -1){
                 colors[nearest] = thread_id;
                 omp_unset_lock(&color_lock);
               } else {

                 int other_thread;
                 #pragma omp critical
                  {
                     other_thread = colors[nearest];
                     omp_set_lock(&locks[other_thread]);
                     omp_set_lock(&locks[thread_id]);
                  }

                  if (merge_waits[(num_threads * other_thread)+other_thread]){
                  //that thread is already trying to merge with me OR some other thread
                  //printf("Thread %d says: someone is already trying to merge with me!\n", thread_id);
                      omp_unset_lock(&color_lock);
                      omp_unset_lock(&locks[other_thread]);
                      omp_unset_lock(&locks[thread_id]);
                      continue;
                  }

                  //otherwise, first merge anything into me that needs to be merged
                  merge_all(dists, colors, merge_waits, thread_id, num_nodes, num_threads);
                  omp_unset_lock(&color_lock);

                  //then mark myself as waiting to merge and release the locks
                  merge_waits[(num_threads*other_thread)+thread_id] = 1;
                  merge_waits[(num_threads*thread_id)+thread_id] = 1;
                  my_cost += dists[offset + nearest];
                  omp_unset_lock(&locks[other_thread]);
                  omp_unset_lock(&locks[thread_id]);

                  omp_set_lock(&total_lock);
                  total_cost += my_cost;
                  omp_unset_lock(&total_lock);
                  break; //busy wait to respawn (break for now)
               }

               my_cost += dists[offset + nearest];
               dists[offset + nearest] = INT_MIN;
               for (int j = 0; j < edges[nearest].size(); j++){
                   int vertex = edges[nearest][j]->endpoint;
                   int weight = edges[nearest][j]->weight;
                   dists[offset + vertex] = std::min(weight, dists[offset + vertex]);
                }
           }
       }
       #pragma omp barrier
       omp_destroy_lock(&locks[thread_id]);
    }
    omp_destroy_lock(&color_lock);
    omp_destroy_lock(&total_lock);
    return total_cost;
}
