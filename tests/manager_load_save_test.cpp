#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <graph/graph.h>
#include <core/manager.h>

int main() {
	std::cout << "Starting Tests \n";

	FerryDB::SingleGraph<int, int, int> graph;

	graph.AddNode(1, 2);
	graph.AddNode(2, 400);
	graph.AddNode(3, 600);

	graph.AddEdge(1, 2, 100);
	graph.AddEdge(1, 3, 200);

	//std::cout << "Graph before serialization \n";
	assert(graph.Get(1) == 2);
	assert(graph.Get(2) == 400);
	assert(graph.Get(3) == 600);

	assert(graph.GetEdgeWeight(1, 2) == 100);
	std::cout << "Updating weights now \n";
	graph.UpdateWeight(1, 2, 300);
	assert(graph.GetEdgeWeight(1, 2) == 300);

	assert(graph.GetOutBoundNodes(1).size() == 2);
	assert(graph.GetInBoundNodes(2).size() == 1);

	FerryDB::ObjectManager<FerryDB::SingleGraph<int, int, int>> Manager{ "MyManager" };
	Manager.Save(graph);
	auto DeserializedGraph = Manager.Load();

	assert(DeserializedGraph.Get(1) == 2);
	assert(DeserializedGraph.Get(2) == 400);
	assert(DeserializedGraph.Get(3) == 600);
	assert(DeserializedGraph.GetEdgeWeight(1, 2) == 300);
	assert(DeserializedGraph.GetOutBoundNodes(1).size() == 2);
	assert(DeserializedGraph.GetInBoundNodes(2).size() == 1);

	std::cout << "Done \n";
}
