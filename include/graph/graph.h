#ifndef GRAPH
#define GRAPH

#include "graph_descriptor.h"
#include <any>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include<iostream>

namespace FerryDB {
	template<class Vertex, typename WeightType = int>
	// Might not need Printable here, but added it in case we do need it
		requires graph::PrintableGraphTypes<Vertex, WeightType> && graph::Hashable<Vertex>&& graph::Hashable<WeightType>
	class SingleGraph {
	public:
		// Added Vertex and VertexID 2 way mapping in-case we need to do 
		// complex operations
		using VertexID = int;
		using Edge = graph::edge_descriptor<VertexID, WeightType>;

	private:
		std::unordered_map<Vertex, VertexID> VertexToIdMapping;
		std::unordered_map<VertexID, Vertex> IdToVertexMapping;
		std::unordered_map<VertexID, std::unordered_set<Edge>> Edges;
		VertexID VertexId = 0;

		void AddEdge(const Edge& edge) {
			Edges[edge.source].insert(edge);
		}

	public:
		SingleGraph() = default;

		SingleGraph(const SingleGraph& Other) {
			IdToVertexMapping = Other.IdToVertexMapping;
			Edges = Other.Edges;
		}

		SingleGraph(SingleGraph&& Other) noexcept {
			IdToVertexMapping = std::move(Other.IdToVertexMapping);
			Edges = std::move(Other.Edges);
		}

		SingleGraph& operator=(const SingleGraph& Other) {
			if (this == &Other) return *this;
			IdToVertexMapping = Other.IdToVertexMapping;
			Edges = Other.Edges;
			return *this;
		}

		SingleGraph& operator=(SingleGraph&& Other) noexcept {
			if (this == &Other) return *this;
			IdToVertexMapping = std::move(Other.IdToVertexMapping);
			Edges = std::move(Other.Edges);
			return *this;
		}

		~SingleGraph() = default;

		void AddNode(Vertex value) {
			IdToVertexMapping[VertexId] = value;
			VertexToIdMapping[value] = VertexId++;
		}

		void AddEdge(const Vertex& FromNode, const Vertex& ToNode, WeightType Weight) {
			if (VertexToIdMapping.find(FromNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("FromNode does not exist.");
			}
			if (VertexToIdMapping.find(ToNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("ToNode does not exist.");
			}
			Edge edge(VertexToIdMapping[FromNode], VertexToIdMapping[ToNode], Weight);
			AddEdge(edge);
		}

		void UpdateWeight(const Vertex& FromNode, const Vertex& ToNode, WeightType new_weight) {
			if (VertexToIdMapping.find(FromNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("FromNode does not exist.");
			}
			if (VertexToIdMapping.find(ToNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("ToNode does not exist.");
			}
			auto FromVertexID = VertexToIdMapping[FromNode];
			auto& edge_set = Edges[FromVertexID];
			for (auto& edge : edge_set) {
				if (edge.destination == VertexToIdMapping[ToNode]) {
					edge.weight = new_weight;
					return;
				}
			}
			throw std::invalid_argument("Edge does not exist.");
		}

		std::unordered_set<Vertex> GetOutBoundNodes(const Vertex& VertexVal) {
			if (VertexToIdMapping.find(VertexVal) == VertexToIdMapping.end()) {
				throw std::invalid_argument("Vertex does not exist.");
			}
			auto VertexId = VertexToIdMapping[VertexVal];
			auto EdgesAtVertex = Edges[VertexId];
			std::unordered_set<Vertex> out_bound_nodes;
			for (const auto& edge : EdgesAtVertex) {
				out_bound_nodes.insert(IdToVertexMapping[edge.destination]);
			}
			return out_bound_nodes;
		}

		std::unordered_set<Vertex> GetInBoundNodes(const Vertex& VertexVal) {
			if (VertexToIdMapping.find(VertexVal) == VertexToIdMapping.end()) {
				throw std::invalid_argument("Vertex does not exist.");
			}
			auto id = VertexToIdMapping[VertexVal];
			std::unordered_set<Vertex> in_bound_nodes;
			for (const auto& [source, edge_set] : Edges) {
				for (const auto& edge : edge_set) {
					if (edge.destination == id) {
						in_bound_nodes.insert(IdToVertexMapping[edge.source]);
					}
				}
			}
			return in_bound_nodes;
		}

		// TODO handle orphan graphs
		void DeleteEdge(const Vertex& FromNode, const Vertex& ToNode) {
			if (VertexToIdMapping.find(FromNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("FromNode does not exist.");
			}
			if (VertexToIdMapping.find(ToNode) == VertexToIdMapping.end()) {
				throw std::invalid_argument("ToNode does not exist.");
			}
			auto VertexID = VertexToIdMapping[FromNode];
			auto EdgesAtVertex = Edges[VertexID];
			std::unordered_set<Vertex> out_bound_nodes;
			for (const auto& edge : EdgesAtVertex) {
				if (edge.destination == VertexToIdMapping[ToNode]) {
					EdgesAtVertex.erase(edge);
					return;
				}
			}
			throw std::invalid_argument("Edge does not exist.");
		}

		// TODO handle orphan graphs
		void DeleteNode(const Vertex& vertex) {
			auto VertexId = VertexToIdMapping[vertex];
			VertexToIdMapping.erase(vertex);
			IdToVertexMapping.erase(VertexId);
			Edges.erase(VertexId);
			for (auto& [source, edge_set] : Edges) {
				std::vector<Edge> to_delete;
				for (const auto& edge : edge_set) {
					if (edge.destination == VertexId) {
						to_delete.push_back(edge);
					}
				}
				for (const auto& edge : to_delete) {
					edge_set.erase(edge);
				}
			}
		}

		void PrintGraphDFS(Vertex startVertex) {
			auto VertexId = VertexToIdMapping[startVertex];
			std::stack<VertexID> stack;
			std::unordered_set<VertexID> visited;
			stack.push(VertexId);

			while (!stack.empty()) {
				auto current = stack.top();
				stack.pop();

				if (visited.find(current) == visited.end()) {
					visited.insert(current);
					std::cout << IdToVertexMapping[current] << " ";

					for (const auto& edge : Edges[current]) {
						if (visited.find(edge.destination) == visited.end()) {
							stack.push(edge.destination);
						}
					}
				}
			}
		}
	};

	template<class Vertex, typename WeightType>
	// Might not need Printable here, but added it in case we do need it
		requires graph::PrintableGraphTypes<Vertex, WeightType> && graph::Hashable<Vertex>&& graph::Hashable<WeightType>
	class Graph {
	private:
		std::unordered_map<std::string, SingleGraph<Vertex, WeightType>> Graphs;
	public:
		using GraphType = SingleGraph<Vertex, WeightType>;

		Graph() = default;

		void AddNode(const std::string& Namespace, const Vertex& Node) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				Graphs[Namespace] = SingleGraph<Vertex, WeightType>();
			}
			Graphs[Namespace].AddNode(Node);
		}

		void AddEdge(const std::string& Namespace, const Vertex& FromNode, const Vertex& ToNode, WeightType Weight) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			Graphs[Namespace].AddEdge(FromNode, ToNode, Weight);
		}

		void UpdateWeight(const std::string& Namespace, const Vertex& FromNode, const Vertex& ToNode, WeightType NewWeight) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			Graphs[Namespace].UpdateWeight(FromNode, ToNode, NewWeight);
		}

		std::unordered_set<Vertex> GetOutBoundNodes(const std::string& Namespace, const Vertex& FromNode) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			return Graphs[Namespace].GetOutBoundNodes(FromNode);
		}

		std::unordered_set<Vertex> GetInBoundNodes(const std::string& Namespace, const Vertex& ToNode) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			return Graphs[Namespace].GetInBoundNodes(ToNode);
		}

		void DeleteEdge(const std::string& Namespace, const Vertex& FromNode, const Vertex& ToNode) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			Graphs[Namespace].DeleteEdge(FromNode, ToNode);
		}

		void PrintGraphDFS(const std::string& Namespace, const Vertex& StartVertex) {
			if (Graphs.find(Namespace) == Graphs.end()) {
				throw std::invalid_argument("Namespace does not exist.");
			}
			Graphs[Namespace].PrintGraphDFS(StartVertex);
		}
	};
}

// Create A Node
// AddNode(Namespace, Graph, Node, NodeData);

// Add an edge between two nodes with optional weight
// AddEdge(Namespace, Graph, FromNode, ToNode, Weight);

// Update the weights between nodes
// UpdateWeight(Namespace, Graph, FromNode, ToNode, NewWeight);

// Get Out Bound Nodes
// GetOutBoundNodes(Namespace, Graph, FromNode);

// Get In Bound Nodes
// GetInBoundNodes(Namespace, Graph, ToNode);

// Delete an Edge - should check for orphan graphs
// DeleteEdge(Namespace, Graph, FromNode, ToNode);

// Delete a node - should check for orphan graphs
// DeleteNode(Namespace, Graph, Node);

#endif // !GRAPH
