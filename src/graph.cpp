#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include "graph.h"

static inline int num_nodes(const Graph& graph) {
    return graph.num_nodes;
}

static inline int num_edges(const Graph& graph) {
    return graph.num_edges;
}

// build a graph from a text file
std::shared_ptr<Graph> load_graph(const char* filename) {
    std::shared_ptr<Graph> G = std::make_shared<Graph>();
    std::vector<std::shared_ptr<flexible_al>> edges; // = new std::vector<flexible_al>;

    std::ifstream graph_file;
    graph_file.open(filename);

    int num_edges;
    int num_nodes;

    std::string buffer;

    // read first two lines of graph file
    // assumes first line can be ignored
    // assumes second line of the form "% {num_edges} {num_nodes}"
    std::getline(graph_file, buffer); // first line
    buffer.clear();
    std::getline(graph_file, buffer); // second line
    std::stringstream parse(buffer);
    char c;
    parse >> c; // dummy var to hold "%"
    parse >> num_edges;
    parse >> num_nodes;

    edges.resize(num_nodes);

    for (int i = 0; i < num_nodes; i++) {
        std::shared_ptr<flexible_al> f = std::make_shared<flexible_al>();
        std::vector<std::shared_ptr<Edge>> n;
        f->neighbors = n;
        f->next = NULL;
        edges[i] = f;
    }

    // now read in edges
    while(!graph_file.eof()) {
        buffer.clear();
        std::getline(graph_file, buffer);

        if (buffer.size() <= 0) continue; // skip blank lines

        // parse edge
        std::stringstream parse(buffer);
        int u, v, wt;

        if (parse.fail()) break;
        parse >> u;
        if (parse.fail()) break;
        parse >> v;
        if (parse.fail()) break;
        parse >> wt;

        // vertices in file are 1-indexed
        // make them 0-indexed
        u--;
        v--;

        // construct edge
        std::shared_ptr<Edge> e1 = std::make_shared<Edge>();
        e1->root = u;
        e1->endpoint = v;
        e1->weight = wt;

        std::shared_ptr<Edge> e2 = std::make_shared<Edge>();
        e2->root = v;
        e2->endpoint = u;
        e2->weight = wt;

        // add edge to both u and v in adjacency list
        edges[u]->neighbors.push_back(e1);
        edges[v]->neighbors.push_back(e2);
    }

    G->num_edges = num_edges;
    G->num_nodes = num_nodes;
    G->edges = edges;
    return G;
}

void print_graph(std::shared_ptr<Graph> G, bool print_weights) {
    std::cout << G->num_nodes << " vertices\n";
    std::cout << G->num_edges << " edges\n";
    for (int i = 0; i < G->num_nodes; i++) {
        std::cout << i << ": <";
        std::shared_ptr<flexible_al> adj_list = G->edges[i];
        
        while (adj_list != NULL) {
            std::vector<std::shared_ptr<Edge>> neighbors = adj_list->neighbors;
            for (int j = 0; j < (int)neighbors.size(); j++) {
                std::shared_ptr<Edge> e = neighbors[j];

                if (j > 0) std::cout << ", ";
                if (print_weights) {
                    std::cout << "(" << e->endpoint << ", " << e->weight << ")";
                } else {
                    std::cout << e->endpoint;
                }
            }
            adj_list = adj_list->next;
        }
        std::cout << ">\n";
    }
}