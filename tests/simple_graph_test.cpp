#include <iostream>
#include <cassert>
#include "graph/graph.h"

int main() {
    std::cout << "Starting Tests \n";
    FerryDB::Graph<int, int, int> graph;

    graph.AddNode("Namespace1", 1, 1);
    graph.AddNode("Namespace1", 2, 2);
    graph.AddNode("Namespace1", 3, 3);

    assert(graph.GetOutBoundNodes("Namespace1", 1).size() == 0);
    graph.AddEdge("Namespace1", 1, 2, 5);
    assert(graph.GetOutBoundNodes("Namespace1", 1).size() == 1);
    graph.AddEdge("Namespace1", 2, 3, 10);
    graph.AddEdge("Namespace1", 1, 3, 10);
    assert(graph.GetOutBoundNodes("Namespace1", 1).size() == 2);
    assert(graph.GetInBoundNodes("Namespace1", 3).size() == 2);
    assert(graph.GetInBoundNodes("Namespace1", 1).size() == 0);

    assert(graph.GetEdgeWeights("Namespace1", 1, 2) == 5);
    graph.UpdateWeight("Namespace1", 1, 2, 8);
    assert(graph.GetEdgeWeights("Namespace1", 1, 2) == 8);


    std::cout << "Done \n";
}
