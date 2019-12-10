#include <string>
#include <cstdlib>

#include "boruvka.h"

int boruvka(std::shared_ptr<Graph> graph){
  int num_nodes = graph->num_nodes;
  int num_components = num_nodes;
  int *component_labels = (int*)malloc(sizeof(int) * num_nodes);
  std::vector<std::shared_ptr<Edge>> best_edges;
  best_edges.resize(num_components);
  for (int i = 0; i < num_nodes; i++){
    component_labels[i] = i;
  }
  int total_cost = 0;
  while(true){
    for (int i = 0; i < num_components; i++) {
      best_edges[i] = NULL;
    }
    //Find cheapest edges
    for (int i = 0; i < num_nodes; i++) {
        std::vector<std::shared_ptr<Edge>> these_edges = graph->edges[i];
        for (int j = 0; j < (int)these_edges.size(); j++){
          std::shared_ptr<Edge> this_edge = these_edges[j];
          if (component_labels[this_edge->root] != component_labels[this_edge->endpoint]) {
              int component = component_labels[i];
              //all the clauses are needed for consistency
              if (best_edges[component] == NULL ||
                      best_edges[component]->weight > this_edge->weight ||
                      (best_edges[component]->weight == this_edge->weight &&
                       best_edges[component]->root > this_edge->root) ||
                      (best_edges[component]->weight == this_edge->weight &&
                       best_edges[component]->root == this_edge->root &&
                       best_edges[component]->endpoint > this_edge->endpoint)) {
                  //lock will be needed in parallel version of this
                  best_edges[component] = this_edge;
              }
          }
        }
    }

    //Merge Edges
    bool all_null = true;
    for (int i = 0; i < num_components; i++) { //Don't trivially parallelize this
      std::shared_ptr<Edge> best_edge = best_edges[i];
      if (best_edge != NULL){
        all_null = false;
        //We need to check again in case two components had the same edge
        if (component_labels[best_edge->root] != component_labels[best_edge->endpoint]) {
          total_cost += best_edge->weight;
          int old_label = std::max(component_labels[best_edge->root], 
                                         component_labels[best_edge->endpoint]);
          int new_label = std::min(component_labels[best_edge->root], 
                                         component_labels[best_edge->endpoint]);
          for (int j = 0; j < num_nodes; j++) {
            if (component_labels[j] == old_label) {
              component_labels[j] = new_label;
            }
          }
        }
      }
    }
    if (all_null) {
      //We are done!
      free(component_labels);
      return total_cost;
    }

    num_components = relabel_components(component_labels, num_components, num_nodes);
  }
}
