#include <string>
#include <cstdlib>

#include "graph.h"

int boruvkas(Graph *graph)
{
    int num_components = graph->num_nodes;
    flexible_al *components = malloc(num_components*sizeof(flexible_al));
    flexible_al **tails = malloc(num_components*sizeof(flexible_al *));
    Edge *best_edges = malloc(num_components*sizeof(Edge));
    int *component_labels = malloc(graph->num_nodes*sizeof(int));
    for (int i=0; i < num_components; i++){
       components[i] = graph->edges[i];
       tails[i] = &components[i];
       component_labels[i] = i;
    }
    int tree_cost = 0;
    while (true) {
        // Find Best Edges
        for (int i=0; i < num_components; i++){
            best_edges[i] = find_best_edge(i, components, num_components);
        }
        // Find Components
        bool any_good = false;
        for (int i=0; i < num_components; i++){ // Don't trivially parallelize
            if (best_edges[i] != NULL){
               any_good = true;
               Edge this_edge = best_edges[i];
               int root_label = component_labels[this_edge->root];
               int endpoint_label = component_lables[this_edge->endpoint];
               int min = std::min(root_label, endpoint_label);
               int child = std::max(root_label, endpoint_label);
               int parent = component_labels[min];
               //Find Components
               component_labels[child] = parent;
               //Merge Components
               tree_cost += this_edge->weight;
               tails[parent]->next = &components[child];
               tails[parent] = tails[child];
            }
        }
        if (!any_good) {
            return tree_cost;
        }
        // Fix Component List
        next_component_num = 0;
        for (int i=0; i < num_components; i++){ //Don't trivially parallelize
            if (component_labels[i] == i){
               components[next_component_num] = components[i];
               next_component_num++;
            }
        }
        for (int i=0; i< next_component_num; i++){
            relabel_edges(components, i, component_labels, num_components);
        }
        num_components = next_component_num;
    }
}

Edge find_best_edge(int i, flexible_al *components, int num_components){
    Edge cheapest_edge = NULL;
    flexible_al *curr = components;
    for (flexible_al *curr = components, curr != null; curr = curr->next) {
        for (int i = 0; i < curr->neighbors.size(); i+=1){
            if ((cheapest_edge == NULL || 
                     curr->neighbors[i]->weight < cheapest_edge.weight) &&
                         curr->neighbors[i]->root != curr->neighbors[i]->endpoint){
                cheapest_edge = curr->neighbors[i];
            }
        }
    }
}

void relabel_edges(flexible_al *components, int i, int *component_labels, int num_components){
   for (flexible_al *curr = components; curr != null; curr = curr->next){
      for (int i = 0; i < curr->neighbors.size(); i+=1){
        int root_num = curr->neighbors[i]->root;
        int end_num = curr->neighbors[i]->endpoint;
        curr->neighbors[i]->root = component_labels[root_num]; //This is shady
        curr->neighbors[i]->endpoint = component_labels[end_num];
      }
   }
}
