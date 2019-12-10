#include <string>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include <omp.h>

#include "parallel_prim.h"
#define MAX_RESPAWN_ATTEMPTS 4

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

inline void merge_all(int *dists, int *colors, bool *merge_waits, int thread_id, int num_nodes, int num_threads){
    int offset_this = num_nodes * thread_id;
    for (int thread = 0; thread < num_threads; thread++){
        if (thread != thread_id && merge_waits[(num_threads * thread_id) + thread]){
            int offset_other = num_nodes * thread;
            //merge the other thread into me
            for (int i = 0; i < num_nodes; i++){
                dists[offset_this + i] = 
                    std::min(dists[offset_this + i], dists[offset_other + i]);
                if (dists[offset_this + i] == INT_MIN){
                    colors[i] = thread_id;
                }
            }
            //let the other thread go
            merge_waits[(num_threads * thread_id) + thread] = 0;
            //printf("Thread %d just let thread %d go at location %d.\n", thread_id, thread, &merge_waits[(num_threads * thread_id)+thread]);
        }
    }
}


int parallel_prims(std::shared_ptr<Graph> graph){
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
    omp_lock_t *locks;
    omp_lock_t color_lock;
    omp_init_lock(&color_lock);
    bool* volatile merge_waits;
    omp_lock_t total_lock;
    omp_init_lock(&total_lock);
    int total_cost = 0;
    omp_lock_t visited_lock;
    omp_init_lock(&visited_lock);
    int total_visited = 0;
    #pragma omp parallel private(thread_id) num_threads(8)
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
       int start_node = (num_nodes / num_threads) * thread_id;
       //Add start_node right away
       dists[offset + start_node] = INT_MIN;
       for (int i = 0; i < edges[start_node].size(); i++){
           dists[offset + (edges[start_node][i]->endpoint)] = edges[start_node][i]->weight;
       }
       colors[start_node] = thread_id;

       #pragma omp barrier
           //add more nodes
            while (true) {
               omp_set_lock(&color_lock);
               omp_set_lock(&locks[thread_id]);
               merge_all(dists, colors, merge_waits, thread_id, num_nodes, num_threads);
               omp_unset_lock(&locks[thread_id]);
               omp_unset_lock(&color_lock);
  
               int nearest = find_nearest(dists, offset, num_nodes);
              
               //printf("Thread %d's nearest neighbor is %d.\n", thread_id, nearest);

               //if there are no more nodes
               if (nearest == -1) {
                   omp_set_lock(&total_lock);
                   total_cost += my_cost;
                   omp_unset_lock(&total_lock);
                   omp_set_lock(&color_lock);
                   omp_set_lock(&locks[thread_id]);
                   merge_all(dists, colors, merge_waits, thread_id, num_nodes, num_threads);
                   merge_waits[(num_threads * thread_id)+thread_id] = 1;
                   omp_unset_lock(&locks[thread_id]);
                   omp_unset_lock(&color_lock);
                   break;
               }

               omp_set_lock(&color_lock);
               if (colors[nearest] == -1){
                 colors[nearest] = thread_id;
                 omp_unset_lock(&color_lock);
                 omp_set_lock(&visited_lock);
                 total_visited += 1;
                 omp_unset_lock(&visited_lock);
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
                      omp_unset_lock(&locks[thread_id]);
                      omp_unset_lock(&locks[other_thread]);
                      omp_unset_lock(&color_lock);
                      continue;
                  }

                  //otherwise, first merge anything into me that needs to be merged
                  merge_all(dists, colors, merge_waits, thread_id, num_nodes, num_threads);

                  //then mark myself as waiting to merge and release the locks
                  merge_waits[(num_threads*other_thread)+thread_id] = 1;
                  merge_waits[(num_threads*thread_id)+thread_id] = 1;
                  omp_unset_lock(&locks[thread_id]);
                  omp_unset_lock(&locks[other_thread]);
                  omp_unset_lock(&color_lock);
                  
                  my_cost += dists[offset + nearest];
                  omp_set_lock(&total_lock);
                  total_cost += my_cost;
                  omp_unset_lock(&total_lock);

                  //busy wait to respawn (break if no repawn)
                  if ((num_nodes - total_visited) < respawn_cutoff){
                      break;
                  } else {
                      while (merge_waits[(num_threads*other_thread) + thread_id]){
                          usleep(10);
                      }
                      //respawn with new start node!
                      int new_start_node = rand() % num_nodes;
                      int num_tries = 0;
                      while (num_tries < MAX_RESPAWN_ATTEMPTS){
                          omp_set_lock(&color_lock);
                          if (colors[new_start_node] == -1){
                              colors[new_start_node] = thread_id;
                              omp_unset_lock(&color_lock);
                              break;
                          }
                          omp_unset_lock(&color_lock);
                          new_start_node = rand() % num_nodes;
                          num_tries++;
                      }
                      if (num_tries >= MAX_RESPAWN_ATTEMPTS){
                      //algorithm is probably basically done
                           break;
                      }
                      //clear dists
                      for (int i = 0; i < num_nodes; i++){
                          dists[offset + i] = INT_MAX;
                      }
                      my_cost = 0;
                     //Add new start node
                     dists[offset + new_start_node] = INT_MIN;
                     for (int i = 0; i < edges[new_start_node].size(); i++){
                         dists[offset + (edges[new_start_node][i]->endpoint)] = edges[new_start_node][i]->weight;
                     }
                     //Mark yourself as alive
                     merge_waits[(num_threads*thread_id)+thread_id] = 0;
                     continue;
                     }
               }

               //This is the case where we don't have to merge
               my_cost += dists[offset + nearest];
               dists[offset + nearest] = INT_MIN;
               for (int j = 0; j < edges[nearest].size(); j++){
                   int vertex = edges[nearest][j]->endpoint;
                   int weight = edges[nearest][j]->weight;
                   dists[offset + vertex] = std::min(weight, dists[offset + vertex]);
                }
           }
       #pragma omp barrier
       omp_destroy_lock(&locks[thread_id]);
    }
    omp_destroy_lock(&color_lock);
    omp_destroy_lock(&total_lock);
    return total_cost;
}
