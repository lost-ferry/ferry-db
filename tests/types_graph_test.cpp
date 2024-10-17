#include <iostream>
#include <cassert>
#include <string>
#include<cstring>
#include "../include/graph/graph.h"

class ComplexTestType: public FerryDB::Serializable::Serializable{
    std::string string;
    std::vector <int> vector;



public:
    ComplexTestType() = default;
    ComplexTestType(std::string string, std::vector <int> vector) : string(string), vector(vector) {}

    std::size_t serialize_size() const {
        return sizeof(string) + sizeof(vector);
    }

    void Serialize(char* data) override {
        std::memcpy(data, &string, sizeof(string));
        std::memcpy(data + sizeof(string), &vector, sizeof(vector));
    }

    void Deserialize(char* data) override {
        std::memcpy(&string, data, sizeof(string));
        std::memcpy(&vector, data + sizeof(string), sizeof(vector));
    }
};

int main() {
    std::cout << "Starting Tests \n";
    std::cout << "Testing with int as VertexData\n";
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


    /*std::cout << "Testing with ComplexTestType as VertexData\n";
    ComplexTestType complexTestType1("Hello", {1, 2, 3, 4, 5});
    ComplexTestType complexTestType2("Hi", {1, 2, 3, 4, 5});
    ComplexTestType complexTestType3("Hi", {1, 2, 3, 4, 5});

    FerryDB::Graph<int, ComplexTestType, int> graph2;

    graph2.AddNode("Namespace1", 1, complexTestType1);
    graph2.AddNode("Namespace1", 2, complexTestType2);
    graph2.AddNode("Namespace1", 3, complexTestType3);

    assert(graph2.GetOutBoundNodes("Namespace1", 1).size() == 0);
    graph2.AddEdge("Namespace1", 1, 2, 5);
    assert(graph2.GetOutBoundNodes("Namespace1", 1).size() == 1);
    graph2.AddEdge("Namespace1", 2, 3, 10);
    graph2.AddEdge("Namespace1", 1, 3, 10);
    assert(graph2.GetOutBoundNodes("Namespace1", 1).size() == 2);
    assert(graph2.GetInBoundNodes("Namespace1", 3).size() == 2);
    assert(graph2.GetInBoundNodes("Namespace1", 1).size() == 0);

    assert(graph2.GetEdgeWeights("Namespace1", 1, 2) == 5);
    graph2.UpdateWeight("Namespace1", 1, 2, 8);
    assert(graph2.GetEdgeWeights("Namespace1", 1, 2) == 8);*/

    // FerryDB::Graph<int, ComplexTestType, int> graph3;

    // graph3.AddNode("Namespace1", 1, &complexTestType1);
    // graph3.AddNode("Namespace1", 2, &complexTestType2);
    // graph3.AddNode("Namespace1", 3, &complexTestType3);

    // assert(graph3.GetOutBoundNodes("Namespace1", 1).size() == 0);
    // graph3.AddEdge("Namespace1", 1, 2, 5);
    // assert(graph3.GetOutBoundNodes("Namespace1", 1).size() == 1);
    // graph3.AddEdge("Namespace1", 2, 3, 10);
    // graph3.AddEdge("Namespace1", 1, 3, 10);
    // assert(graph3.GetOutBoundNodes("Namespace1", 1).size() == 2);
    // assert(graph3.GetInBoundNodes("Namespace1", 3).size() == 2);
    // assert(graph3.GetInBoundNodes("Namespace1", 1).size() == 0);

    // assert(graph3.GetEdgeWeights("Namespace1", 1, 2) == 5);
    // graph3.UpdateWeight("Namespace1", 1, 2, 8);
    // assert(graph3.GetEdgeWeights("Namespace1", 1, 2) == 8);

    std::cout << "Done \n";
}