static inline int num_nodes(const Graph& graph) {
    return graph.num_nodes;
}

static inline int num_edges(const Graph& graph) {
    return graph.num_edges;
}

void read_graph_file(std::ifstream& file)
{
  std::string buffer;
  int idx = 0;
  while(!file.eof())
  {
    buffer.clear();
    std::getline(file, buffer);

    if (buffer.size() > 0 && buffer[0] == '#')
        continue;

    std::stringstream parse(buffer);
    while (!parse.fail()) {
        int v;
        parse >> v;
        if (parse.fail())
        {
            break;
        }
        scratch[idx] = v;
        idx++;
    }
  }
}

Graph& load_graph(const char* filename) {
    Graph *G = new Graph;
    std::vector<flexible_al> edges = new flexible_al;

    std::ifstream graph_file;
    graph_file.open(filename);

    int num_edges;
    int num_nodes;

    std::string buffer;
    while(!graph_file.eof()) {
        buffer.clear();
        std::getline(graph_file, buffer);

        if (buffer.size() <= 0) contine; // skip blank lines

        if (buffer[0] == '%') {
            // read graph file headers
            if (buffer.size() > 0 && std::isdigit(buffer[2]))
                continue; // ignore first line
            
            // read second line of the form "% {num_edges} {num_nodes}"
            std::stringstream parse(buffer);
            char c;
            parse >> c; // dummy var to hold "%"
            parse >> num_edges;
            parse >> num_nodes;

            G.num_edges = num_edges;
            G.num_nodes = num_nodes;

        } else {
            // parse edge
            std::stringstream parse(buffer);
            int u, v, wt;

            if (parse.fail()) break;
            parse >> u;
            if (parse.fail()) break;
            parse >> v;
            if (parse.fail()) break;
            parse >> wt;
        }
    }

    G.edges = edges;
}