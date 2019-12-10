#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <vector>
#include <memory>

class Edge {
public:
    int root;
    int endpoint;
    int weight;
};

// Graph implemented with an adjacency list
class Graph {
public:
    // Number of edges in the graph
    int num_edges;
    // Number of vertices in the graph
    int num_nodes;

    // Adjacency list
    std::vector<std::vector<std::shared_ptr<Edge>>> edges;
};

// Flexible adjacency list representation
class flexible_al {
public:
    // component number
    int original_label;

    // Array of neighbors
    std::vector<std::shared_ptr<Edge>> neighbors;

    // Next array of neighbors in flexible adjacency list
    std::shared_ptr<flexible_al> next;
};

// Graph implemented with a flexible adjacency list
class FAL_Graph {
public:
    // Number of edges in the graph
    int num_edges;
    // Number of vertices in the graph
    int num_nodes;

    // Flexible adjacency list
    std::vector<std::shared_ptr<flexible_al>> edges;

    // Store pointers to last FALs for each vertex
    //std::vector<std::shared_ptr<flexible_al>> lasts;
};

class Supervertex {
public:
    // component number
    int label;
    std::vector<std::shared_ptr<Edge>> edges;
};

class merge_graph {
public:
    // Number of edges in the graph
    int num_edges;
    // Number of vertices in the graph
    int num_nodes;

    // Adjacency list
    std::vector<Supervertex> supervertices;
};

/* Getters */
//static inline int num_nodes(const Graph&);
//static inline int num_edges(const Graph&);

//static inline const Vertex* begin(const Graph, Vertex);
//static inline const Vertex* end(const Graph, Vertex);
//static inline int degree(const Graph, Vertex);


/* IO */
std::shared_ptr<Graph> load_graph(const char*);
std::shared_ptr<FAL_Graph> load_FAL_graph(const char*);

//Graph load_graph_binary(const char* filename);
//void store_graph_binary(const char* filename, Graph);

void print_graph(std::shared_ptr<Graph>, bool print_weights);
void print_FAL_graph(std::shared_ptr<FAL_Graph>, bool print_weights);
void print_merge_graph(std::shared_ptr<merge_graph>, bool print_weights);


int relabel_components(int *, int, int);

#endif