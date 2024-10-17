#ifndef INCLUDE_GRAPH_GRAPH_H_
#define INCLUDE_GRAPH_GRAPH_H_

#include "graph_descriptor.h"
#include <any>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <memory>
#include <variant>

namespace FerryDB {
template <class VertexID, class VertexData, typename WeightType = int>
  requires graph::Hashable<VertexID> && graph::Hashable<WeightType>
class SingleGraph {
public:
  using Edge = graph::edge_descriptor<VertexID, WeightType>;
  using VertexDataPtr = VertexData;

private:
  std::unordered_map<VertexID, VertexDataPtr> IdToVertexMapping;
  std::unordered_map<VertexID, std::unordered_set<Edge>> Edges;
  VertexID VertexId = 0;

  void AddEdge(const Edge &Edge) { 
    Edges[Edge.source].insert(Edge);
  }

public:
  SingleGraph() = default;

  SingleGraph(const SingleGraph &Other) {
    IdToVertexMapping = Other.IdToVertexMapping;
    Edges = Other.Edges;
  }

  SingleGraph(SingleGraph &&Other) noexcept {
    IdToVertexMapping = std::move(Other.IdToVertexMapping);
    Edges = std::move(Other.Edges);
  }

  SingleGraph &operator=(const SingleGraph &Other) {
    if (this == &Other)
      return *this;
    IdToVertexMapping = Other.IdToVertexMapping;
    Edges = Other.Edges;
    return *this;
  }

  SingleGraph &operator=(SingleGraph &&Other) noexcept {
    if (this == &Other)
      return *this;
    IdToVertexMapping = std::move(Other.IdToVertexMapping);
    Edges = std::move(Other.Edges);
    return *this;
  }

  ~SingleGraph() = default;

  VertexData Get(const VertexID& Id) {
    if (IdToVertexMapping.find(Id) == IdToVertexMapping.end()) {
        throw std::invalid_argument("Vertex does not exist.");
    }

    if (std::holds_alternative<VertexData>(IdToVertexMapping[Id])) {
        // Return the direct value
        return std::get<VertexData>(IdToVertexMapping[Id]);
    } else if (std::holds_alternative<std::shared_ptr<VertexData>>(IdToVertexMapping[Id])) {
        // Dereference the shared pointer to get the value
        return *(std::get<std::shared_ptr<VertexData>>(IdToVertexMapping[Id]));
    } else {
        throw std::runtime_error("Unexpected type in IdToVertexMapping.");
    }
  }

  void AddNode(const VertexID& Id, const VertexData& Value) { IdToVertexMapping[Id] = Value; }

  void AddNode(const VertexID &Id, VertexData* Value) { IdToVertexMapping[Id] = Value; }

  void AddEdge(const VertexID &FromVertexId, const VertexID &ToVertexId,
               WeightType Weight) {
    if (IdToVertexMapping.find(FromVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("FromVertexId does not exist.");
    }
    if (IdToVertexMapping.find(ToVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("ToVertexId does not exist.");
    }
    Edge Edge_(FromVertexId, ToVertexId, Weight);
    AddEdge(Edge_);
  }

  void UpdateWeight(const VertexID &FromVertexId, const VertexID &ToVertexId,
                    WeightType NewWeight) {
    if (IdToVertexMapping.find(FromVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("FromVertexId does not exist.");
    }
    if (IdToVertexMapping.find(ToVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("ToVertexId does not exist.");
    }
    auto &EdgeSet = Edges[FromVertexId];
    for (auto it = EdgeSet.begin(); it != EdgeSet.end(); ++it) {
      if (it->destination == ToVertexId) {
        Edge ModifiedEdge = *it;
        EdgeSet.erase(it);

        ModifiedEdge.weight = NewWeight;

        EdgeSet.insert(ModifiedEdge);
        return;
      }
    }
    throw std::invalid_argument("Edge does not exist.");
  }

  std::vector<std::pair<VertexID, VertexDataPtr>> GetOutBoundNodes(const VertexID &VertexId) {
    if (IdToVertexMapping.find(VertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("Vertex does not exist.");
    }
    auto EdgesAtVertex = Edges[VertexId];
    std::vector<std::pair<VertexID, VertexDataPtr>> OutBoundNodes;
    for (const auto &Edge_ : EdgesAtVertex) {
      OutBoundNodes.push_back(std::make_pair(Edge_.destination, IdToVertexMapping[Edge_.destination]));
    }
    return OutBoundNodes;
  }

  std::vector<std::pair<VertexID, VertexDataPtr>> GetInBoundNodes(const VertexID &VertexId) {
    if (IdToVertexMapping.find(VertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("Vertex does not exist.");
    }
    std::vector<std::pair<VertexID, VertexDataPtr>> InBoundNodes;
    for (const auto &[Source, EdgeSet] : Edges) {
      for (const auto &Edge : EdgeSet) {
        if (Edge.destination == VertexId) {
          InBoundNodes.push_back(std::make_pair(Edge.source, IdToVertexMapping[Edge.source]));
        }
      }
    }
    return InBoundNodes;
  }

  // TODO(AnishDeshmukh1999): handle orphan graphs
  void DeleteEdge(const VertexID &FromVertexId, const VertexID &ToVertexId) {
    if (IdToVertexMapping.find(FromVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("FromVertexId does not exist.");
    }
    if (IdToVertexMapping.find(ToVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("ToVertexId does not exist.");
    }
    auto EdgesAtVertex = Edges[VertexId];
    for (const auto &Edge : EdgesAtVertex) {
      if (Edge.destination == IdToVertexMapping[ToVertexId]) {
        EdgesAtVertex.erase(Edge);
        return;
      }
    }
    throw std::invalid_argument("Edge does not exist.");
  }

  // TODO(AnishDeshmukh1999): handle orphan graphs
  void DeleteNode(const VertexID &VertexId) {
    IdToVertexMapping.erase(VertexId);
    Edges.erase(VertexId);
    for (auto &[Source, EdgeSet] : Edges) {
      std::vector<Edge> ToDelete;
      for (const auto &Edge : EdgeSet) {
        if (Edge.destination == VertexId) {
          ToDelete.push_back(Edge);
        }
      }
      for (const auto &Edge : ToDelete) {
        EdgeSet.erase(Edge);
      }
    }
  }

  void PrintGraphDFS(VertexID StartVertexId) {
    std::stack<VertexID> Stack;
    std::unordered_set<VertexID> Visited;
    Stack.push(StartVertexId);

    while (!Stack.empty()) {
      auto CurrentVertexId = Stack.top();
      Stack.pop();

      if (Visited.find(CurrentVertexId) == Visited.end()) {
        Visited.insert(CurrentVertexId);
        std::cout << CurrentVertexId << " ";

        for (const auto &Edge : Edges[CurrentVertexId]) {
          if (Visited.find(Edge.destination) == Visited.end()) {
            Stack.push(Edge.destination);
          }
        }
      }
    }
  }

  WeightType GetEdgeWeights(const VertexID &FromVertexId, const VertexID &ToVertexId) {
    if (IdToVertexMapping.find(FromVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("FromVertexId does not exist.");
    }
    if (IdToVertexMapping.find(ToVertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("ToVertexId does not exist.");
    }
    auto &EdgeSet = Edges[FromVertexId];
    for (const auto &Edge : EdgeSet) {
      if (Edge.destination == ToVertexId) {
        return Edge.weight;
      }
    }
    throw std::invalid_argument("Edge does not exist.");
  }
};

template <class VertexID, class VertexData, typename WeightType>
// Might not need Printable here, but added it in case we do need it
  requires graph::Hashable<VertexID> && graph::Hashable<WeightType>
class Graph {
private:
  std::unordered_map<std::string, SingleGraph<VertexID, VertexData, WeightType>> Graphs;

public:
  using VertexDataPtr = VertexData;
  using GraphType = SingleGraph<VertexID, VertexData, WeightType>;

  Graph() = default;

  void AddNode(const std::string &Namespace, const VertexID &VertexId, const VertexData &Vertex) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
    }
    Graphs[Namespace].AddNode(VertexId, Vertex);
  }

  void AddNode(const std::string &Namespace, const VertexID &VertexId, VertexData *Vertex) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
    }
    Graphs[Namespace].AddNode(VertexId, Vertex);
  }

  void AddEdge(const std::string &Namespace, const VertexID &FromVertexId,
               const VertexID &ToVertexId, WeightType Weight) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].AddEdge(FromVertexId, ToVertexId, Weight);
  }

  void UpdateWeight(const std::string &Namespace, const VertexID &FromVertexId,
                    const VertexID &ToVertexId, WeightType NewWeight) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].UpdateWeight(FromVertexId, ToVertexId, NewWeight);
  }

  std::vector<std::pair<VertexID, VertexDataPtr>> GetOutBoundNodes(const std::string &Namespace,
                                              const VertexID &FromVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].GetOutBoundNodes(FromVertexId);
  }

  std::vector<std::pair<VertexID, VertexDataPtr>> GetInBoundNodes(const std::string &Namespace,
                                             const VertexID &ToVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].GetInBoundNodes(ToVertexId);
  }

  void DeleteEdge(const std::string &Namespace, const VertexID &FromVertexId,
                  const VertexID &ToVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].DeleteEdge(FromVertexId, ToVertexId);
  }

  void PrintGraphDFS(const std::string &Namespace, const VertexID &StartVertex) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].PrintGraphDFS(StartVertex);
  }

  WeightType GetEdgeWeights(const std::string &Namespace,
                            const VertexID &FromVertexId, const VertexID &ToVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].GetEdgeWeights(FromVertexId, ToVertexId);
  }

  size_t NodeCount(const std::string &Namespace) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].IdToVertexMapping.size();
  }

  std::shared_ptr<VertexDataPtr> Get(const std::string &Namespace, const VertexID &Id) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].Get(Id);
  }
};
} // namespace FerryDB
#endif // INCLUDE_GRAPH_GRAPH_H_
