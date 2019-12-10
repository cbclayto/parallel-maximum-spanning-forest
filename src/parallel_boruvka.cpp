#include <string>
#include <cstdlib>
#include <omp.h>

#include "parallel_boruvka.h"
#include "timing.h"

int get_component(int v, int *temp_labels) {
    while (temp_labels[v] != v) {
      v = temp_labels[v];
    }
  return v;
}

int parallel_boruvka(std::shared_ptr<Graph> graph){
  int num_nodes = graph->num_nodes;
  int num_components = num_nodes;
  int *component_labels = (int*)malloc(sizeof(int) * num_nodes);
  std::vector<std::shared_ptr<Edge>> best_edges;
  best_edges.resize(num_components);
  for (int i = 0; i < num_nodes; i++){
   component_labels[i] = i;
  }
  int total_cost = 0;

  Timer t;
  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
  while(true){
    t.reset();
    //#pragma omp parallel for
    for (int i = 0; i < num_components; i++){
      best_edges[i] = NULL;
    }
    time1 += t.elapsed();

    t.reset();
    //Find cheapest edges
    for (int i = 0; i < num_nodes; i++){
      std::vector<std::shared_ptr<Edge>> these_edges = graph->edges[i];
      for (int j = 0; j < (int)these_edges.size(); j++){
        std::shared_ptr<Edge> this_edge = these_edges[j];
        if (component_labels[this_edge->root] != component_labels[this_edge->endpoint]) {
          int component = component_labels[i];
          std::shared_ptr<Edge> best = best_edges[component];
          //all the clauses are needed for consistency
          if (best == NULL ||
              best->weight > this_edge->weight ||
              (best->weight == this_edge->weight &&
               best->root > this_edge->root) ||
              (best->weight == this_edge->weight &&
               best->root == this_edge->root &&
               best->endpoint > this_edge->endpoint)) {
            //lock will be needed in parallel version of this
            best_edges[component] = this_edge;
          }
        }
      }
    }
    time2 += t.elapsed();

    //Merge Edges
    t.reset();
    for (int i = 0; i < num_components; i++) { //Don't trivially parallelize this
      std::shared_ptr<Edge> best_edge = best_edges[i];
      if (best_edge != NULL) {
        //We need to check again in case two components had the same edge
        if (component_labels[best_edge->root] != component_labels[best_edge->endpoint]) {
          total_cost += best_edge->weight;
          int old_label = std::max(component_labels[best_edge->root], 
                                         component_labels[best_edge->endpoint]);
          int new_label = std::min(component_labels[best_edge->root], 
                                         component_labels[best_edge->endpoint]);
               
          #pragma omp parallel for schedule(static)
          for (int j = 0; j < num_nodes; j++){
            if (component_labels[j] == old_label){
              component_labels[j] = new_label;
            }
          }
        }
      }
    }
    time3 += t.elapsed();

    // re-enumerate components
    num_components = relabel_components(component_labels, num_components, num_nodes);

    if (num_components == 1) {
    //We are done!
        printf("\tSection 1: %.6fms\n", time1);
        printf("\tSection 2: %.6fms\n", time2);
        printf("\tSection 3: %.6fms\n", time3);
        free(component_labels);
        return total_cost;
    }
  }
}
