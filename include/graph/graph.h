#ifndef INCLUDE_GRAPH_GRAPH_H_
#define INCLUDE_GRAPH_GRAPH_H_

#include <any>
#include <core/serializable.h>
#include <graph/graph_descriptor.h>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace FerryDB {
template <class VertexID, class VertexData, typename WeightType = int>
  requires graph::Hashable<VertexID> && graph::Hashable<WeightType> &&
           SerializableConcepts::Serializable<VertexData> &&
           SerializableConcepts::Serializable<VertexID> &&
           SerializableConcepts::Serializable<WeightType>
class SingleGraph : public Serializable::Serializable {
public:
  using Edge = graph::edge_descriptor<VertexID, WeightType>;
  using VertexDataPtr = VertexData;

private:
  std::unordered_map<VertexID, VertexDataPtr> IdToVertexMapping;
  std::unordered_map<VertexID, std::unordered_set<Edge>> Edges;
  size_t EdgeCount = 0;

  void AddEdge(const Edge &Edge) {
    Edges[Edge.source].insert(Edge);
    EdgeCount++;
  }

  size_t GetVertexDataSize(const VertexData &data) const {
    if constexpr (std::is_fundamental<VertexData>::value) {
      std::cout << "In fundamental vertex data size: " << sizeof(VertexData)
                << "\n";
      return sizeof(VertexData);
    }
    else if(SerializableConcepts::is_string_like<VertexData>::value){
      std::cout << "In string like vertex data size: " << data.size() << "\n";
      return sizeof(size_t) + data.size();
    } else {
      return data.SerializerSize();
    }
  }

  size_t GetVertexIdSize(const VertexID &id) const {
    if constexpr (std::is_fundamental<VertexID>::value) {
      std::cout << "In fundamental vertex id size: " << sizeof(VertexID)
                << "\n";
      return sizeof(VertexID);
    } else {
      return id.SerializerSize();
    }
  }

  size_t GetWeightTypeSize(const WeightType &weight) const {
    if constexpr (std::is_fundamental<WeightType>::value) {
      return sizeof(WeightType);
    } else {
      return weight.SerializerSize();
    }
  }

  size_t GetEdgeSize(const Edge &edge) const {
    size_t result = GetVertexIdSize(edge.source) +
                    GetVertexIdSize(edge.destination) +
                    GetWeightTypeSize(edge.weight);
    std::cout << "Edge size: " << result << "\n";
    return result;
  }

  size_t GetAllEdgesSize() const {
    size_t EdgeDataSize = 0;
    for (const auto &[Id, EdgeSet] : Edges) {
      for (const auto &Edge_ : EdgeSet) {
        EdgeDataSize += GetEdgeSize(Edge_);
      }
    }
    std::cout << "All edges size: " << EdgeDataSize << "\n";
    return EdgeDataSize;
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

  std::size_t SerializerSize() const override {
    size_t HeaderSize = sizeof(graph::GraphHeader);
    std::cout << "Header size: " << HeaderSize << "\n";
    size_t VertexIdDataSize = 0;
    for (const auto &[Id, Data] : IdToVertexMapping) {
      std::cout << "Size of VertexHeader: " << sizeof(graph::VertexHeader)
                << "\n";
      VertexIdDataSize += GetVertexDataSize(Data) + GetVertexIdSize(Id) +
                          sizeof(graph::VertexHeader);
    }
    std::cout << "Vertex id data size: " << VertexIdDataSize << "\n";
    size_t EdgeDataSize = GetAllEdgesSize();
    std::cout << "Edge data size: " << EdgeDataSize << "\n";
    size_t TotalSize = HeaderSize + VertexIdDataSize + EdgeDataSize;
    std::cout << "Total size: " << TotalSize << "\n";
    return TotalSize;
  }

  // Assume Buffer is of size SerializerSize()
  void Serialize(char *Buffer) override {
    graph::GraphHeader Header;
    Header.MagicNumber = graph::MAGIC_NUMBER;
    Header.VertexCount = IdToVertexMapping.size();
    Header.EdgeCount = EdgeCount;
    Header.VertexStartOffset = sizeof(graph::GraphHeader);
    Header.EdgeStartOffset = Header.VertexStartOffset + GetAllEdgesSize();

    char *currentPtr = Buffer;
    std::memcpy(currentPtr, &Header, sizeof(graph::GraphHeader));
    currentPtr += sizeof(graph::GraphHeader);

    std::cout << "Current Pointer after serializing header: " << static_cast<void*>(currentPtr) << "\n";

    int VertexNumber = 1;
    for (const auto &[Id, Data] : IdToVertexMapping) {
      graph::VertexHeader VHeader{};
      VHeader.VertexNumber = VertexNumber++;
      // TODO fix VertexIdOffset. Not being used currently
      VHeader.VertexIdOffset =
          sizeof(graph::GraphHeader) + sizeof(graph::VertexHeader);
      VHeader.VertexDataOffset = VHeader.VertexIdOffset + GetVertexIdSize(Id);
      std::memcpy(currentPtr, &VHeader, sizeof(graph::VertexHeader));

      currentPtr += sizeof(graph::VertexHeader);

      std::cout << "Current Pointer after serializing vertex header: " << static_cast<void*>(currentPtr) << "\n";

      // Vertex ID
      if constexpr (std::is_fundamental<VertexID>::value) {
        std::memcpy(currentPtr, &Id, sizeof(VertexID));
        currentPtr += sizeof(VertexID);
      } else {
        Id.Serialize(currentPtr);
        currentPtr += Id.SerializerSize();
      }

      // Vertex Data
      if constexpr (std::is_fundamental<VertexData>::value) {
        std::memcpy(currentPtr, &Data, sizeof(VertexData));
        currentPtr += sizeof(VertexData);
      } else if (SerializableConcepts::is_string_like<VertexData>::value) {
        size_t str_len = Data.size();
        std::memcpy(currentPtr, &str_len, sizeof(size_t));
        currentPtr += sizeof(size_t);
        std::memcpy(currentPtr, Data.c_str(), str_len);
        currentPtr += str_len;
      } else {
        Data.Serialize(currentPtr);
        currentPtr += Data.SerializerSize();
      }
    }

    std::cout << "Current Pointer after serializing all vertices: " << static_cast<void*>(currentPtr) << "\n";

    // TODO use internal IDs as numbers for random access
    size_t EdgeNumber = 1;
    for(const auto& [Id, EdgeSet]: Edges){
      for(const auto& Edge_: EdgeSet){
        graph::EdgeHeader EHeader{};
        EHeader.EdgeNumber = EdgeNumber++;
        // TODO replace 0s with actual offsets and use it for random access
        EHeader.SourceVertexIdOffset = 0;
        EHeader.DestinationVertexIdOffset = 0;
        EHeader.WeightOffset = 0;

        std::cout << "Edge Header Information: " << EHeader.EdgeNumber << " " 
          << EHeader.SourceVertexIdOffset << " " 
          << EHeader.DestinationVertexIdOffset << " " << EHeader.WeightOffset << "\n";

        std::memcpy(currentPtr, &EHeader, sizeof(graph::EdgeHeader));
        currentPtr += sizeof(graph::EdgeHeader);

        // Source Vertex ID
        if constexpr (std::is_fundamental<VertexID>::value) {
          std::memcpy(currentPtr, &Edge_.source, sizeof(VertexID));
          currentPtr += sizeof(VertexID);
        } else {
          Edge_.source.Serialize(currentPtr);
          currentPtr += Edge_.source.SerializerSize();
        }

        // Destination Vertex ID
        if constexpr (std::is_fundamental<VertexID>::value) {
          std::memcpy(currentPtr, &Edge_.destination, sizeof(VertexID));
          currentPtr += sizeof(VertexID);
        } else {
          Edge_.destination.Serialize(currentPtr);
          currentPtr += Edge_.destination.SerializerSize();
        }

        // Weight
        if constexpr (std::is_fundamental<WeightType>::value) {
          std::memcpy(currentPtr, &Edge_.weight, sizeof(WeightType));
          currentPtr += sizeof(WeightType);
        } else {
          Edge_.weight.Serialize(currentPtr);
          currentPtr += Edge_.weight.SerializerSize();
        }
      }
    }
    std::cout << "reached end of serialization\n" << static_cast<void*>(currentPtr) << "\n";
  }

  void Deserialize(char *Buffer) override {

    const char *currentPtr = Buffer;

    graph::GraphHeader Header;
    std::memcpy(&Header, currentPtr, sizeof(graph::GraphHeader));
    currentPtr += sizeof(graph::GraphHeader);

    std::cout << "Current Pointer after de-serializing header: " << static_cast<const void*>(currentPtr) << "\n";

    IdToVertexMapping.clear();

    std::cout << "Header information: " << Header.MagicNumber << " "
              << Header.VertexCount << " " << Header.EdgeCount << "\n";

    for (int i = 0; i < Header.VertexCount; ++i) {
      // Read vertex header
      graph::VertexHeader VHeader;
      std::memcpy(&VHeader, currentPtr, sizeof(graph::VertexHeader));
      currentPtr += sizeof(graph::VertexHeader);

      // Step 3: Deserialize the Vertex ID
      VertexID Id;
      if constexpr (std::is_fundamental<VertexID>::value) {
        std::memcpy(&Id, currentPtr, sizeof(VertexID));
        currentPtr += sizeof(VertexID);
      } else {
        Id.Deserialize(currentPtr);
        currentPtr += Id.SerializerSize();
      }

      // Step 4: Deserialize the Vertex Data
      VertexData Data;
      if constexpr (std::is_fundamental<VertexData>::value) {
        std::memcpy(&Data, currentPtr, sizeof(VertexData));
        currentPtr += sizeof(VertexData);
      } else if (SerializableConcepts::is_string_like<VertexData>::value) {
        size_t str_len = 0;
        std::memcpy(&str_len, currentPtr, sizeof(size_t));
        currentPtr += sizeof(size_t);
        std::memcpy(&Data, currentPtr, str_len);
        currentPtr += str_len;
      } else {
        Data.Deserialize(currentPtr);
        currentPtr += Data.SerializerSize();
      }

      // Add the deserialized data back to the graph's vertex mapping
      IdToVertexMapping[Id] = Data;

      std::cout << "Vertex ID: " << Id << " Vertex Data: " << Data << "\n";
    }

    std::cout << "Current Pointer after de-serializing all vertices: " << static_cast<const void*>(currentPtr) << "\n";

    for(int i = 0; i < Header.EdgeCount; i++){
      std::cout << "I value: " << i << " " << EdgeCount << "\n";
      graph::EdgeHeader EHeader;
      std::memcpy(&EHeader, currentPtr, sizeof(graph::EdgeHeader));
      currentPtr += sizeof(graph::EdgeHeader);

      std::cout << "Edge Header Information: " << EHeader.EdgeNumber << " " 
        << EHeader.SourceVertexIdOffset << " " 
        << EHeader.DestinationVertexIdOffset << " " << EHeader.WeightOffset << "\n";

      // Source Vertex ID
      VertexID Source;
      if constexpr (std::is_fundamental<VertexID>::value) {
        std::memcpy(&Source, currentPtr, sizeof(VertexID));
        currentPtr += sizeof(VertexID);
      } else {
        Source.Deserialize(currentPtr);
        currentPtr += Source.SerializerSize();
      }

      std::cout << "Source: " << Source << "\n";

      // Destination Vertex ID
      VertexID Destination;
      if constexpr (std::is_fundamental<VertexID>::value) {
        std::memcpy(&Destination, currentPtr, sizeof(VertexID));
        currentPtr += sizeof(VertexID);
      } else {
        Destination.Deserialize(currentPtr);
        currentPtr += Destination.SerializerSize();
      }

      std::cout << "Destination: " << Destination << "\n";

      // Weight
      WeightType Weight;
      if constexpr (std::is_fundamental<WeightType>::value) {
        std::memcpy(&Weight, currentPtr, sizeof(WeightType));
        currentPtr += sizeof(WeightType);
      } else {
        Weight.Deserialize(currentPtr);
        currentPtr += Weight.SerializerSize();
      }

      std::cout << "Weight: " << Weight << "\n";

      Edge Edge_(Source, Destination, Weight);
      AddEdge(Edge_);
    }

    std::cout << "reached end of de-serialization\n" << static_cast<const void*>(currentPtr) << "\n";
  }

  VertexData Get(const VertexID &Id) {
    if (IdToVertexMapping.find(Id) == IdToVertexMapping.end()) {
      throw std::invalid_argument("Vertex does not exist.");
    }

    return IdToVertexMapping[Id];
  }

  void AddNode(const VertexID &Id, const VertexData &Value) {
    IdToVertexMapping[Id] = Value;
  }

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

  std::vector<std::pair<VertexID, VertexDataPtr>>
  GetOutBoundNodes(const VertexID &VertexId) {
    if (IdToVertexMapping.find(VertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("Vertex does not exist.");
    }
    auto EdgesAtVertex = Edges[VertexId];
    std::vector<std::pair<VertexID, VertexDataPtr>> OutBoundNodes;
    for (const auto &Edge_ : EdgesAtVertex) {
      OutBoundNodes.push_back(std::make_pair(
          Edge_.destination, IdToVertexMapping[Edge_.destination]));
    }
    return OutBoundNodes;
  }

  std::vector<std::pair<VertexID, VertexDataPtr>>
  GetInBoundNodes(const VertexID &VertexId) {
    if (IdToVertexMapping.find(VertexId) == IdToVertexMapping.end()) {
      throw std::invalid_argument("Vertex does not exist.");
    }
    std::vector<std::pair<VertexID, VertexDataPtr>> InBoundNodes;
    for (const auto &[Source, EdgeSet] : Edges) {
      for (const auto &Edge : EdgeSet) {
        if (Edge.destination == VertexId) {
          InBoundNodes.push_back(
              std::make_pair(Edge.source, IdToVertexMapping[Edge.source]));
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
    auto EdgesAtVertex = Edges[FromVertexId];
    for (const auto &Edge : EdgesAtVertex) {
      if (Edge.destination == IdToVertexMapping[ToVertexId]) {
        EdgesAtVertex.erase(Edge);
        EdgeCount--;
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

  WeightType GetEdgeWeights(const VertexID &FromVertexId,
                            const VertexID &ToVertexId) {
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
  requires graph::Hashable<VertexID> && graph::Hashable<WeightType> &&
           SerializableConcepts::Serializable<VertexData> &&
           SerializableConcepts::Serializable<VertexID> &&
           SerializableConcepts::Serializable<WeightType>
class Graph {
private:
  std::unordered_map<std::string, SingleGraph<VertexID, VertexData, WeightType>>
      Graphs;

public:
  using VertexDataPtr = VertexData;
  using GraphType = SingleGraph<VertexID, VertexData, WeightType>;

  Graph() = default;

  void AddNode(const std::string &Namespace, const VertexID &VertexId,
               const VertexData &Vertex) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
    }
    Graphs[Namespace].AddNode(VertexId, Vertex);
  }

  void AddNode(const std::string &Namespace, const VertexID &VertexId,
               VertexData *Vertex) {
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

  std::vector<std::pair<VertexID, VertexDataPtr>>
  GetOutBoundNodes(const std::string &Namespace, const VertexID &FromVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].GetOutBoundNodes(FromVertexId);
  }

  std::vector<std::pair<VertexID, VertexDataPtr>>
  GetInBoundNodes(const std::string &Namespace, const VertexID &ToVertexId) {
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

  void PrintGraphDFS(const std::string &Namespace,
                     const VertexID &StartVertex) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].PrintGraphDFS(StartVertex);
  }

  WeightType GetEdgeWeights(const std::string &Namespace,
                            const VertexID &FromVertexId,
                            const VertexID &ToVertexId) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].GetEdgeWeights(FromVertexId, ToVertexId);
  }

  size_t NodeCount(const std::string &Namespace) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].IdToVertexMapping.SerializerSize();
  }

  VertexDataPtr Get(const std::string &Namespace, const VertexID &Id) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].Get(Id);
  }

  size_t size(const std::string &Namespace) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    return Graphs[Namespace].SerializerSize();
  }

  void Serialize(const std::string &Namespace, char *Buffer) {
    if (Graphs.find(Namespace) == Graphs.end()) {
      throw std::invalid_argument("Namespace does not exist.");
    }
    Graphs[Namespace].Serialize(Buffer);
  }

  void Deserialize(const std::string &Namespace, char *Buffer) {
    Graphs[Namespace].Deserialize(Buffer);
  }
};
}; // namespace FerryDB
#endif // INCLUDE_GRAPH_GRAPH_H_