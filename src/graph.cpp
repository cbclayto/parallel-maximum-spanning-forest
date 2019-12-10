#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include "graph.h"

std::vector<int> read_graph_file_header(std::ifstream *graph_file, std::string *buffer) {
    // read first two lines of graph file
    // assumes first line can be ignored
    // assumes second line of the form "% {num_edges} {num_nodes}"
    int num_nodes, num_edges;

    std::getline(*graph_file, *buffer); // first line
    buffer->clear();
    std::getline(*graph_file, *buffer); // second line
    std::stringstream parse(*buffer);
    char c;
    parse >> c; // dummy var to hold "%"
    parse >> num_edges;
    parse >> num_nodes;

    std::vector<int> ret;
    ret.resize(2);

    ret[0] = num_edges;
    ret[1] = num_nodes;
    return ret;
}

std::vector<std::shared_ptr<Edge>> parse_edges(std::ifstream *graph_file, std::string *buffer) {
    buffer->clear();
    std::getline(*graph_file, *buffer);

    std::vector<std::shared_ptr<Edge>> ret;
    ret.resize(0);

    if (buffer->size() <= 0) {
        // skip blank lines
        return ret;
    }

    // parse edge
    std::stringstream parse(*buffer);
    int u, v, wt;

    if (parse.fail()) return ret;
    parse >> u;
    if (parse.fail()) return ret;
    parse >> v;
    if (parse.fail()) return ret;
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

    ret.resize(2);
    ret[0] = e1;
    ret[1] = e2;
    return ret;
}

// check
bool inGraph(std::vector<std::shared_ptr<Edge>> neighbors, int v) {
    //std::vector<std::shared_ptr<Edge>> neighbors = G->edges[u];
    for (int i = 0; i < (int)neighbors.size(); i++) {
        std::shared_ptr<Edge> edge = neighbors[i];
        if (edge->endpoint == v)
            return true;
    }
    return false;
}

// build a graph from a text file
std::shared_ptr<Graph> load_graph(const char* filename) {
    std::shared_ptr<Graph> G = std::make_shared<Graph>();
    std::vector<std::vector<std::shared_ptr<Edge>>> edges;

    std::ifstream graph_file;
    graph_file.open(filename);

    if (graph_file.fail()) {
        printf("Error: File does not exist.\n");
        exit(1);
    }

    std::string buffer;

    std::vector<int> headers = read_graph_file_header(&graph_file, &buffer);
    G->num_nodes = headers[1];
    int edge_count = 0;

    edges.resize(G->num_nodes);

    // read in edges
    while(!graph_file.eof()) {
        std::vector<std::shared_ptr<Edge>> es = parse_edges(&graph_file, &buffer);

        if ((int)es.size() <= 0) continue; // skip blank lines
        std::shared_ptr<Edge> e1 = es[0];
        std::shared_ptr<Edge> e2 = es[1];

        // ignore duplicate edges (allows directed graphs to be parsed as undirected)
        if (inGraph(edges[e1->root], e1->endpoint)) continue;
        edge_count++;

        // add edge to both u and v in adjacency list
        edges[e1->root].push_back(e1);
        edges[e2->root].push_back(e2);
    }

    if (edge_count != headers[0]) {
        std::cout << "\nThe input graph was directed." <<
        " It has been transformed to be undirected.\n" << headers[0] <<
        " edges --> " << edge_count << " edges\n\n";
    }
    G->num_edges = edge_count;
    G->edges = edges;
    return G;
}

// build a FAL_graph from a text file
std::shared_ptr<FAL_Graph> load_FAL_graph(const char* filename) {
    std::shared_ptr<FAL_Graph> G = std::make_shared<FAL_Graph>();
    std::vector<std::shared_ptr<flexible_al>> edges;

    std::ifstream graph_file;
    graph_file.open(filename);

    std::string buffer;

    std::vector<int> headers = read_graph_file_header(&graph_file, &buffer);
    G->num_edges = headers[0];
    G->num_nodes = headers[1];

    edges.resize(G->num_nodes);

    // initialize flexible adjacency lists
    for (int i = 0; i < G->num_nodes; i++) {
        std::shared_ptr<flexible_al> f = std::make_shared<flexible_al>();
        std::vector<std::shared_ptr<Edge>> n;
        f->original_label = i;
        f->neighbors = n;
        f->next = NULL;
        edges[i] = f;
    }

    // now read in edges
    while(!graph_file.eof()) {
        std::vector<std::shared_ptr<Edge>> es = parse_edges(&graph_file, &buffer);

        if ((int)es.size() <= 0) continue; // skip blank lines
        std::shared_ptr<Edge> e1 = es[0];
        std::shared_ptr<Edge> e2 = es[1];

        // add edge to both u and v in adjacency list
        edges[e1->root]->neighbors.push_back(e1);
        edges[e2->root]->neighbors.push_back(e2);
    }

    G->edges = edges;
    //G->lasts = edges; // starts out with FALs of length 1
    return G;
}

void print_graph(std::shared_ptr<Graph> G, bool print_weights) {
    std::cout << G->num_nodes << " vertices\n";
    std::cout << G->num_edges << " edges\n";
    for (int i = 0; i < G->num_nodes; i++) {
        std::cout << i << ": <";
        std::vector<std::shared_ptr<Edge>> neighbors = G->edges[i];
        for (int j = 0; j < (int)neighbors.size(); j++) {
            std::shared_ptr<Edge> e = neighbors[j];

            if (j > 0) std::cout << ", ";
            if (print_weights) {
                std::cout << "(" << e->endpoint << ", " << e->weight << ")";
            } else {
                std::cout << e->endpoint;
            }
        }
        std::cout << ">\n";
    }
    std::cout << "\n";
}

void print_FAL_graph(std::shared_ptr<FAL_Graph> G, bool print_weights) {
    std::cout << G->num_nodes << " vertices\n";
    std::cout << G->num_edges << " edges\n";
    for (int i = 0; i < (int)G->edges.size(); i++) {
        std::cout << i << ": <";
        std::shared_ptr<flexible_al> adj_list = G->edges[i];
        
        while (adj_list != NULL) {
            std::vector<std::shared_ptr<Edge>> neighbors = adj_list->neighbors;
            for (int j = 0; j < (int)neighbors.size(); j++) {
                std::shared_ptr<Edge> e = neighbors[j];

                std::cout << ", ";
                if (print_weights) {
                    std::cout << "(" << e->root << ", " << e->endpoint << ", " << e->weight << ")";
                } else {
                    std::cout << "(" << e->root << ", " << e->endpoint << ")";
                }
            }
            adj_list = adj_list->next;
        }
        std::cout << ">\n";
    }
}

void print_merge_graph(std::shared_ptr<merge_graph> G, bool print_weights) {
    std::cout << G->num_nodes << " vertices\n";
    std::cout << G->num_edges << " edges\n";
    for (int i = 0; i < (int)G->supervertices.size(); i++) {
        Supervertex S = G->supervertices[i];
        std::cout << S.label << ": <";
        std::vector<std::shared_ptr<Edge>> neighbors = S.edges;
        for (int j = 0; j < (int)neighbors.size(); j++) {
            std::shared_ptr<Edge> e = neighbors[j];
            if (e->root != S.label) printf("Graph Error!\n");

            if (j > 0) std::cout << ", ";
            if (print_weights) {
                std::cout << "(" << e->endpoint << ", " << e->weight << ")";
            } else {
                std::cout << e->endpoint;
            }
        }
        std::cout << ">\n";
    }
    std::cout << "\n";
}

int relabel_components(int *component_labels, int n, int num_nodes) {
  int component_count = 0;
  int *new_labels = (int*)malloc(sizeof(int) * n);
  int old_label;
  for (int i = 0; i < n; ++i) {
    new_labels[i] = -1;
  }

  for (int i = 0; i < num_nodes; ++i) {
    old_label = component_labels[i];
    if (new_labels[old_label] == -1) {
      new_labels[old_label] = component_count;
      component_count++;
    }
    component_labels[i] = new_labels[old_label];
  }
  free(new_labels);
  return component_count;
}