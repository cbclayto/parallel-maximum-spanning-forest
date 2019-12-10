#include <string>
#include <cstdlib>
#include <algorithm>

#include "merge_boruvka.h"
#include "timing.h"

std::shared_ptr<merge_graph> graph_to_mergegraph(std::shared_ptr<Graph> G) {
  std::shared_ptr<merge_graph> result = std::make_shared<merge_graph>();
  std::vector<Supervertex> S;
  S.resize(G->num_nodes);
  for (int i = 0; i < G->num_nodes; i++) {
    std::vector<std::shared_ptr<Edge>> neighbors = G->edges[i];
    Supervertex V;
    V.edges = neighbors;
    V.label = i;
    S[i] = V;
  }
  result->supervertices = S;
  result->num_nodes = G->num_nodes;
  result->num_edges = G->num_edges;
  return result;
}

int relabel_vertices(std::shared_ptr<merge_graph> graph, int num_components) {
  int *component_labels = (int*)malloc(sizeof(int) * num_components);
  for (int i = 0; i < num_components; i++) {
    component_labels[i] = graph->supervertices[i].label;
  }

  int new_component_count = relabel_components(component_labels, num_components, num_components);

  // write new labels into graph
  for (int i = 0; i < num_components; i++) {
    graph->supervertices[i].label = component_labels[i];
  }

  free(component_labels);
  return new_component_count;
}

bool compare_labels(Supervertex U, Supervertex V) {
  return U.label < V.label;
}

// used to sort edges with endpoint as the primary key and weight as the secondary key
bool compare_edges(std::shared_ptr<Edge> e1, std::shared_ptr<Edge> e2) {
  return (e1->endpoint < e2->endpoint) || 
         ((e1->endpoint == e2->endpoint) && (e1->weight < e2->weight));
}

inline bool same_edge(std::shared_ptr<Edge> e1, std::shared_ptr<Edge> e2) {
  return (e1->root == e2->root) && (e1->endpoint == e2->endpoint);
}

inline bool is_self_loop(std::shared_ptr<Edge> e) {
  return e->root == e->endpoint;
}

// merges two edge vectors, but removes self-loops and multi-edges
std::vector<std::shared_ptr<Edge>> merge(std::vector<std::shared_ptr<Edge>> A, 
                                         std::vector<std::shared_ptr<Edge>> B) {
  std::vector<std::shared_ptr<Edge>> C;
  C.resize((int)A.size() + (int)B.size());

  int a = 0;
  int b = 0;
  int c = 0;
  std::shared_ptr<Edge> Ea, Eb;
  while (a < (int)A.size() && b < (int)B.size()) {
    Ea = A[a];
    Eb = B[b];
    if (compare_edges(Ea, Eb)) {
      // Ea < Eb
      if (!is_self_loop(Ea) && (c == 0 || !same_edge(C[c-1], Ea))) {
        C[c] = Ea;
        c++;
      }
      a++;
    } else {
      // Eb < Ea
      if (!is_self_loop(Eb) && (c == 0 || !same_edge(C[c-1], Eb))) {
        C[c] = Eb;
        c++;
      }
      b++;
    }
  }

  // add the remaining edges from A or B
  while (a < (int)A.size()) {
    // A has remaining elements
    Ea = A[a];
    if (!is_self_loop(Ea) && (c == 0 || !same_edge(C[c-1], Ea))) {
      C[c] = Ea;
      c++;
    }
    a++;
  }
  while (b < (int)B.size()) {
    // B has remaining elements
    Eb = B[b];
    if (!is_self_loop(Eb) && (c == 0 || !same_edge(C[c-1], Eb))) {
      C[c] = Eb;
      c++;
    }
    b++;
  }

  // resize and return
  C.resize(c);
  return C;
}





int merging_boruvka(std::shared_ptr<Graph> input) {
  int num_components = input->num_nodes;
  bool *visited = (bool*)malloc(sizeof(bool) * num_components);
  int *cycle_weights = (int*)malloc(sizeof(int) * num_components);
  int *merge_counts = (int*)malloc(sizeof(int) * num_components);
  std::vector<std::shared_ptr<Edge>> best_edges;
  best_edges.resize(num_components);

  std::shared_ptr<merge_graph> graph = graph_to_mergegraph(input);

  int total_cost = 0;

  Timer t, t_total;
  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
  while(true) {
    // SECTION 1
    // Find cheapest edges
    t.reset();
    for (int i = 0; i < num_components; i++) {
      std::shared_ptr<Edge> best_edge = NULL;
      for (int j = 0; j < (int)graph->supervertices[i].edges.size(); j++){
        std::shared_ptr<Edge> edge = graph->supervertices[i].edges[j];
        if (best_edge == NULL || edge->weight < best_edge->weight) {
          best_edge = edge;
        }
      }
      best_edges[i] = best_edge;
    }
    time1 += t.elapsed();



    // SECTION 2
    // Identify connected components
    
    // reset cycle weights
    t.reset();
    for (int i = 0; i < num_components; i++) {
      cycle_weights[i] = 0;
    }
    for (int vertex = 0; vertex < num_components; vertex++) {
      //reset visited array
      for (int i = 0; i < num_components; i++) {
        visited[i] = false;
      }

      // mark current vertex
      visited[vertex] = true;

      // perform DFS from each vertex; identify vertex in cycle
      int next = best_edges[vertex]->endpoint;
      while (!(visited[next])) {
        visited[next] = true;
        next = best_edges[next]->endpoint;
      }
      int cycle_start = next;

      // identify root of component as smallest vertex in cycle
      int root = next;
      next = best_edges[next]->endpoint;
      while (next != cycle_start) {
        if (next < root) {
          root = next;
        }
        next = best_edges[next]->endpoint;
      }

      // update component label
      graph->supervertices[vertex].label = root;

      // record weight of edge in cycle
      cycle_weights[root] = best_edges[root]->weight;
    }

    // SECTION 2.1
    // Update MST weight
    for (int i = 0; i < num_components; i++) {
      total_cost += best_edges[i]->weight;
      // subtract cycle weights to not double count edges
      total_cost -= cycle_weights[i];
    }

    // SECTION 2.2
    // Re-enumerate vertices and relabel edges to match
    int new_num_components = relabel_vertices(graph, num_components);
    for (int i = 0; i < num_components; i++) {
      Supervertex S = graph->supervertices[i];
      int root_label = S.label;
      for (int j = 0; j < (int)S.edges.size(); j++) {
        std::shared_ptr<Edge> edge = S.edges[j];
        int old_endpoint = edge->endpoint;
        int new_endpoint = graph->supervertices[old_endpoint].label;
        edge->root = root_label;
        edge->endpoint = new_endpoint;
      }
    }
    time2 += t.elapsed();



    // SECTION 3
    // Merge connected components into supervertices

    t.reset();
    // sort by supervertex labels
    std::sort(graph->supervertices.begin(), graph->supervertices.end(), compare_labels);
    
    // reset merge counts
    for (int i = 0; i < new_num_components; i++) {
      merge_counts[i] = 0;
    }

    // sort neighbors of each vertex by their endpoints
    for (int i = 0; i < num_components; i++) {
      std::sort(graph->supervertices[i].edges.begin(), graph->supervertices[i].edges.end(), compare_edges);
      merge_counts[graph->supervertices[i].label]++; // keep track of component counts
    }

    // scan to get component offsets
    for (int i = 1; i < new_num_components; i++) {
      merge_counts[i] += merge_counts[i-1];
    }

    // merge components together
    // this works because edge vectors are sorted and vertices to be merged are contiguous
    for (int i = 0; i < new_num_components; i++) {
      int start;
      if (i == 0) {
        start = 0;
      } else {
        start = merge_counts[i-1];
      }
      for (int idx = start+1; idx < merge_counts[i]; idx++) {
        graph->supervertices[start].edges = merge(graph->supervertices[start].edges, 
                                                  graph->supervertices[idx].edges);
      }
    }
    for (int i = 1; i < new_num_components; i++) {
      graph->supervertices[i] = graph->supervertices[merge_counts[i-1]];
    }

    time3 += t.elapsed();


    num_components = new_num_components;
    graph->supervertices.resize(num_components);

    if (num_components == 1) {
      // We are done!
      free(visited);
      free(cycle_weights);
      free(merge_counts);

      double time = t_total.elapsed();
      printf("\tSection 1: %.6fms\n", time1);
      printf("\tSection 2: %.6fms\n", time2);
      printf("\tSection 3: %.6fms\n", time3);
      printf("\tMST weight: %d\n", total_cost);
      printf("\tTime: %.6fms\n\n", time);

      return total_cost;
    }
  }
}






// paralell version of the above code
int parallel_merging_boruvka(std::shared_ptr<Graph> input) {
  int num_components = input->num_nodes;
  bool *visited;
  int *cycle_weights = (int*)malloc(sizeof(int) * num_components);
  int *merge_counts = (int*)malloc(sizeof(int) * num_components);
  std::vector<std::shared_ptr<Edge>> best_edges;
  best_edges.resize(num_components);

  std::shared_ptr<merge_graph> graph = graph_to_mergegraph(input);

  int total_cost = 0;

  Timer t, t_total;
  double time1 = 0;
  double time2 = 0;
  double time3 = 0;
  while(true) {
    
    // SECTION 1
    // Find cheapest edges
    t.reset();
    #pragma omp parallel for
    for (int i = 0; i < num_components; i++) {
      std::shared_ptr<Edge> best_edge = NULL;
      for (int j = 0; j < (int)graph->supervertices[i].edges.size(); j++){
        std::shared_ptr<Edge> edge = graph->supervertices[i].edges[j];
        if (best_edge == NULL || edge->weight < best_edge->weight) {
          best_edge = edge;
        }
      }
      best_edges[i] = best_edge;
    }
    time1 += t.elapsed();



    // SECTION 2
    // Identify connected components
    // can be done in parallel if 'visited' is not global
    
    // reset cycle weights
    t.reset();
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
      int next = best_edges[vertex]->endpoint;
      while (!(visited[next])) {
        visited[next] = true;
        next = best_edges[next]->endpoint;
      }
      int cycle_start = next;

      // identify root of component as smallest vertex in cycle
      int root = next;
      next = best_edges[next]->endpoint;
      while (next != cycle_start) {
        if (next < root) {
          root = next;
        }
        next = best_edges[next]->endpoint;
      }

      // update component label
      graph->supervertices[vertex].label = root;

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
    // Re-enumerate vertices and relabel edges to match
    int new_num_components = relabel_vertices(graph, num_components);
    for (int i = 0; i < num_components; i++) {
      Supervertex S = graph->supervertices[i];
      int root_label = S.label;
      for (int j = 0; j < (int)S.edges.size(); j++) {
        std::shared_ptr<Edge> edge = S.edges[j];
        int old_endpoint = edge->endpoint;
        int new_endpoint = graph->supervertices[old_endpoint].label;
        edge->root = root_label;
        edge->endpoint = new_endpoint;
      }
    }
    time2 += t.elapsed();





    // SECTION 3
    // Merge connected components into supervertices

    t.reset();
    // sort by supervertex labels
    std::sort(graph->supervertices.begin(), graph->supervertices.end(), compare_labels);
    
    // reset merge counts
    for (int i = 0; i < new_num_components; i++) {
      merge_counts[i] = 0;
    }

    // sort neighbors of each vertex by their endpoints
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_components; i++) {
      std::sort(graph->supervertices[i].edges.begin(), graph->supervertices[i].edges.end(), compare_edges);
    }

    // keep track of component counts
    // cannot be combined with previous loop due to race condition
    for (int i = 0; i < num_components; i++) {
      merge_counts[graph->supervertices[i].label]++; 
    }

    // scan to get component offsets
    for (int i = 1; i < new_num_components; i++) {
      merge_counts[i] += merge_counts[i-1];
    }

    // merge components together
    // this works because edge vectors are sorted and vertices to be merged are contiguous
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < new_num_components; i++) {
      int start;
      if (i == 0) {
        start = 0;
      } else {
        start = merge_counts[i-1];
      }
      for (int idx = start+1; idx < merge_counts[i]; idx++) {
        graph->supervertices[start].edges = merge(graph->supervertices[start].edges, 
                                                  graph->supervertices[idx].edges);
      }
    }
    for (int i = 1; i < new_num_components; i++) {
      graph->supervertices[i] = graph->supervertices[merge_counts[i-1]];
    }

    time3 += t.elapsed();


    num_components = new_num_components;
    graph->supervertices.resize(num_components);

    if (num_components == 1) {
      // We are done!
      free(cycle_weights);
      free(merge_counts);

      double time = t_total.elapsed();
      printf("\tSection 1: %.6fms\n", time1);
      printf("\tSection 2: %.6fms\n", time2);
      printf("\tSection 3: %.6fms\n", time3);
      printf("\tMST weight: %d\n", total_cost);
      printf("\tTime: %.6fms\n\n", time);

      return total_cost;
    }
  }
}









