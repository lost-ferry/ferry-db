#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "graph/graph.h"

template <typename T>
using is_string_like = std::disjunction<
    std::is_same<std::decay_t<T>, std::string>,
    std::is_same<std::decay_t<T>, std::__cxx11::basic_string<char>>>;

int main() {
    std::cout << "Starting Tests \n";

    FerryDB::Graph<int, int, int> graph;

    graph.AddNode("Namespace1", 1, 2);
    graph.AddNode("Namespace1", 2, 400);
    graph.AddNode("Namespace1", 3, 600);

    graph.AddEdge("Namespace1", 1, 2, 100);
    graph.AddEdge("Namespace1", 1, 3, 200);

    std::cout << "Graph before serialization \n";
    assert(graph.Get("Namespace1", 1) == 2);
    assert(graph.Get("Namespace1", 2) == 400);
    assert(graph.Get("Namespace1", 3) == 600);
    assert(graph.GetOutBoundNodes("Namespace1", 1).size() == 2);

    size_t size = graph.size("Namespace1");
    std::vector<char> buffer(size); // Use vector for automatic memory management
    graph.Serialize("Namespace1", buffer.data());

    std::cout.write(buffer.data(), size);
    std::cout << "\n";

    FerryDB::Graph<int, int, int> graph2;
    graph2.Deserialize("Namespace1", buffer.data());
    std::cout << "Graph after de-serialization \n";
    assert(graph2.Get("Namespace1", 1) == 2);
    assert(graph2.Get("Namespace1", 2) == 400);
    assert(graph2.Get("Namespace1", 3) == 600);
    assert(graph2.GetOutBoundNodes("Namespace1", 1).size() == 2);

    std::cout << "Done \n";
}
