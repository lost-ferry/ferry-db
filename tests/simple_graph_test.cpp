#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <graph/graph.h>

int main() {
    std::cout << "Starting Tests \n";

    FerryDB::SingleGraph<int, int, int> graph;

    graph.AddNode(1, 2);
    graph.AddNode( 2, 400);
    graph.AddNode( 3, 600);

    graph.AddEdge( 1, 2, 100);
    graph.AddEdge( 1, 3, 200);

    std::cout << "Graph before serialization \n";
    assert(graph.Get( 1) == 2);
    assert(graph.Get( 2) == 400);
    assert(graph.Get( 3) == 600);
    assert(graph.GetOutBoundNodes( 1).size() == 2);
}
