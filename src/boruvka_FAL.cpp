#include <string>
#include <cstdlib>
#include <algorithm>
#include <omp.h>

#include "boruvka_FAL.h"
#include "timing.h"

// component labels must be accessed by comparison function
static int *component_labels;

inline bool is_self_loop(std::shared_ptr<Edge> e, int *component_labels) {
  return component_labels[e->root] == component_labels[e->endpoint];
}

// get the updated component label from its original component label
inline int get_cmp_idx(int orig, int *component_labels) {
  return component_labels[orig];
}

bool compare_FAL(std::shared_ptr<flexible_al> F1, std::shared_ptr<flexible_al> F2) {
  return get_cmp_idx(F1->original_label, component_labels) < get_cmp_idx(F2->original_label, component_labels);
}

std::shared_ptr<flexible_al> get_last_FAL(std::shared_ptr<flexible_al> F) {
  while (F->next != NULL) {
    F = F->next;
  }
  return F;
}



int FAL_boruvka(std::shared_ptr<FAL_Graph> graph) {
  int num_components = graph->num_nodes;
  int num_nodes = graph->num_nodes;
  component_labels = (int*)malloc(num_components * sizeof(int));
  int *temp_labels = (int*)malloc(num_components * sizeof(int));
  bool *visited;
  int *cycle_weights = (int*)malloc(sizeof(int) * num_components);
  int *merge_counts = (int*)malloc(sizeof(int) * num_components);
  std::vector<std::shared_ptr<Edge>> best_edges;
  best_edges.resize(num_components);
  
  for (int i=0; i < num_components; i++) {
     component_labels[i] = i;
  }
  int total_cost = 0;
  Timer t;
  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
  while (true) {

    // SECTION 1
    // Find Best Edges
    t.reset();
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_components; i++){
      std::shared_ptr<Edge> best_edge = NULL;
      for (std::shared_ptr<flexible_al> curr = graph->edges[i]; curr != NULL; curr = curr->next) {
        for (int j = 0; j < curr->neighbors.size(); j++) {
          if (!is_self_loop(curr->neighbors[j], component_labels) && 
              (best_edge == NULL || curr->neighbors[j]->weight < best_edge->weight ||
              (curr->neighbors[j]->weight == best_edge->weight && 
                get_cmp_idx(curr->neighbors[j]->endpoint, component_labels) < get_cmp_idx(best_edge->endpoint, component_labels)))) {
            best_edge = curr->neighbors[j];
          }
        }
      }
      best_edges[i] = best_edge;
    }
    time1 += t.elapsed();



    // SECTION 2
    // Identify connected components
    t.reset();
    // reset cycle weights
    for (int i = 0; i < num_components; i++) {
      cycle_weights[i] = 0;
    }

    #pragma omp parallel private(visited)
    {
    visited = (bool*)malloc(sizeof(bool) * num_components);
    #pragma omp for
    for (int vertex = 0; vertex < num_components; vertex++) {
      //reset visited array
      for (int i = 0; i < num_components; i++) {
        visited[i] = false;
      }

      // mark current vertex
      visited[vertex] = true;

      // perform DFS from each vertex; identify vertex in cycle
      int next = get_cmp_idx(best_edges[vertex]->endpoint, component_labels);
      while (!(visited[next])) {
        visited[next] = true;
        next = get_cmp_idx(best_edges[next]->endpoint, component_labels);
      }
      int cycle_start = next;

      // identify root of component as smallest vertex in cycle
      int root = next;
      next = get_cmp_idx(best_edges[next]->endpoint, component_labels);
      while (next != cycle_start) {
        if (next < root) {
          root = next;
        }
        next = get_cmp_idx(best_edges[next]->endpoint, component_labels);
      }

      // store label to later update component label
      temp_labels[vertex] = root;

      // record weight of edge in cycle
      cycle_weights[root] = best_edges[root]->weight;
    }
    free(visited);
    }

    // SECTION 2.1
    // Update MST weight
    for (int i = 0; i < num_components; i++) {
      total_cost += best_edges[i]->weight;
      // subtract cycle weights to not double count edges
      total_cost -= cycle_weights[i];
    }

    // SECTION 2.2
    // Re-enumerate vertices
    for (int i = 0; i < num_nodes; i++) {
      component_labels[i] = temp_labels[component_labels[i]];
    }

    int new_num_components = relabel_components(component_labels, num_components, num_nodes);
    time2 += t.elapsed();





    // SECTION 3
    // Connect Components
    t.reset();
    // sort by supervertex labels
    std::sort(graph->edges.begin(), graph->edges.end(), compare_FAL);

    // reset merge counts
    for (int i = 0; i < new_num_components; i++) {
      merge_counts[i] = 0;
    }
    // keep track of component counts
    for (int i = 0; i < num_components; i++) {
      merge_counts[get_cmp_idx(graph->edges[i]->original_label, component_labels)]++;
    }
    // scan to get component offsets
    for (int i = 1; i < new_num_components; i++) {
      merge_counts[i] += merge_counts[i-1];
    }

    // append all FALs in newly connected components
    #pragma omp parallel for
    for (int component = 0; component < new_num_components; component++) {
      int start;
      if (component == 0) {
        start = 0;
      } else {
        start = merge_counts[component-1];
      }
      std::shared_ptr<flexible_al> last = get_last_FAL(graph->edges[start]);
      for (int idx = start+1; idx < merge_counts[component]; idx++) {
        last->next = graph->edges[idx];
        last = get_last_FAL(last->next);
      }
    }

    // rearrange graph (do not parallelize)
    for (int i = 1; i < new_num_components; i++) {
      graph->edges[i] = graph->edges[merge_counts[i-1]];
    }
    time3 += t.elapsed();

    num_components = new_num_components;
    graph->edges.resize(num_components);

    if (num_components == 1) {
      // We are done!
      free(visited);
      free(cycle_weights);
      free(merge_counts);

      printf("\tSection 1: %.6fms\n", time1);
      printf("\tSection 2: %.6fms\n", time2);
      printf("\tSection 3: %.6fms\n", time3);
      return total_cost;
    }
  }
}