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
		requires graph::Hashable<VertexID>&& graph::Hashable<WeightType>&&
	SerializableConcepts::Serializable<VertexData>&&
		SerializableConcepts::Serializable<VertexID>&&
		SerializableConcepts::Serializable<WeightType>
		class SingleGraph : public Serializable::Serializable {
		public:
			using EdgeType = graph::edge_descriptor<WeightType>;
			using VertexType = graph::vertex_descriptor<VertexID, VertexData>;

		private:
			std::unordered_map<VertexID, int> VertexToInternalIdMapping;
			std::unordered_map<int, VertexType> InternalIdToVertexTypeMapping;
			std::unordered_map<int, std::unordered_set<EdgeType>> Edges;
			int VertexInternalIDIterator = 0;
			int EdgeInternalIDIterator = 0;

			void AddEdge(const EdgeType& Edge) {
				Edges[Edge.SourceInternalVertexID].insert(Edge);
				EdgeInternalIDIterator++;
			}

			size_t GetVertexDataSize(const VertexData& Data) const {
				if constexpr (std::is_fundamental<VertexData>::value) {
					return sizeof(VertexData);
				}
				//else if (SerializableConcepts::is_string_like<VertexData>::value) {
				//	return sizeof(size_t) + Data.size();
				//}
				else {
					return Data.SerializerSize();
				}
			}

			size_t GetVertexIdSize(const VertexID& Id) const {
				if constexpr (std::is_fundamental<VertexID>::value) {
					return sizeof(VertexID);
				}
				else {
					return Id.SerializerSize();
				}
			}

			size_t GetWeightTypeSize(const WeightType& weight) const {
				if constexpr (std::is_fundamental<WeightType>::value) {
					return sizeof(WeightType);
				}
				else {
					return weight.SerializerSize();
				}
			}

			size_t GetEdgeSize(const EdgeType& edge) const {
				size_t result = GetVertexIdSize(edge.SourceInternalVertexID) +
					GetVertexIdSize(edge.DestinationInternalVertexID) +
					GetWeightTypeSize(edge.weight);
				return result;
			}

			size_t GetAllEdgesSize() const {
				size_t EdgeDataSize = 0;
				for (const auto& [Id, EdgeSet] : Edges) {
					for (const auto& Edge_ : EdgeSet) {
						EdgeDataSize += sizeof(graph::EdgeHeader) + GetEdgeSize(Edge_);
					}
				}
				return EdgeDataSize;
			}

			size_t GetAllVertexSize() const {
				size_t VertexIdDataSize = 0;
				for (const auto& [Id, Data] : InternalIdToVertexTypeMapping) {
					VertexIdDataSize += GetVertexDataSize(Data.VertexValue_) + GetVertexIdSize(Data.VertexID_) +
						sizeof(graph::VertexHeader);
				}
				return VertexIdDataSize;
			}

			struct IdMappingStruct {
				VertexID Id;
				int InternalId;
				size_t VertexInternalIdOffset;
				size_t VertexInternalIdDataSize;
			};

			size_t GetIdMappingSize() const {
				size_t CurrentMappingSize = sizeof(graph::IdMappingHeader);
				for (const auto& [Id, Data] : VertexToInternalIdMapping) {
					if constexpr (std::is_fundamental<VertexID>::value) {
						CurrentMappingSize += sizeof(VertexID);
					}
					else {
						CurrentMappingSize += Id.SerializerSize();
					}
					CurrentMappingSize += sizeof(int) + 2 * sizeof(size_t);
				}
				return CurrentMappingSize;
			}

		public:
			SingleGraph() = default;
			// TODO test copy and move constructors
			//SingleGraph(const SingleGraph& Other) {
			//	InternalIdToVertexTypeMapping = Other.InternalIdToVertexTypeMapping;
			//	Edges = Other.Edges;
			//}

			//SingleGraph(SingleGraph&& Other) noexcept {
			//	InternalIdToVertexTypeMapping = std::move(Other.InternalIdToVertexTypeMapping);
			//	Edges = std::move(Other.Edges);
			//}

			//SingleGraph& operator=(const SingleGraph& Other) {
			//	if (this == &Other)
			//		return *this;
			//	InternalIdToVertexTypeMapping = Other.InternalIdToVertexTypeMapping;
			//	Edges = Other.Edges;
			//	return *this;
			//}
			//SingleGraph& operator=(SingleGraph&& Other) noexcept {
			//	if (this == &Other)
			//		return *this;
			//	InternalIdToVertexTypeMapping = std::move(Other.InternalIdToVertexTypeMapping);
			//	Edges = std::move(Other.Edges);
			//	return *this;
			//}

			~SingleGraph() = default;

			std::size_t SerializerSize() const override {
				size_t HeaderSize = sizeof(graph::GraphHeader);
				size_t EdgeDataSize = GetAllEdgesSize();
				size_t VertexIdDataSize = GetAllVertexSize();
				size_t IdMappingSize = GetIdMappingSize();
				size_t TotalSize = HeaderSize + VertexIdDataSize + EdgeDataSize + IdMappingSize;
				return TotalSize;
			}

			// Assume Buffer is of size SerializerSize()
			std::vector<char> Serialize() override {
				size_t Count = 0;
				size_t Size = SerializerSize();

				std::vector<char> Buffer(Size);
				graph::GraphHeader GraphHeader_;
				GraphHeader_.MagicNumber = SerializerTags::MagicNumber::WEIGHTED_GRAPH;
				GraphHeader_.VertexCount = InternalIdToVertexTypeMapping.size();
				GraphHeader_.EdgeCount = EdgeInternalIDIterator;
				GraphHeader_.VertexStartOffset = sizeof(graph::GraphHeader);
				GraphHeader_.EdgeStartOffset = GraphHeader_.VertexStartOffset + GetAllVertexSize();

				char* HeaderPtr = Buffer.data();
				char* CurrentPtr = Buffer.data() + sizeof(graph::GraphHeader);

				// Pair of Offset and Size
				std::unordered_map<int, std::pair<size_t, size_t>> InternalIdToOffsetMapping;

				for (const auto& [Id, Data] : InternalIdToVertexTypeMapping) {
					auto Size = 0;
					graph::VertexHeader VHeader{};
					VHeader.VertexNumber = Data.InternalID;
					// Relative offsets
					VHeader.VertexIdOffset =
						sizeof(graph::VertexHeader);
					VHeader.VertexDataOffset = VHeader.VertexIdOffset + GetVertexIdSize(Id);
					std::memcpy(CurrentPtr, &VHeader, sizeof(graph::VertexHeader));

					CurrentPtr += sizeof(graph::VertexHeader);
					Size += sizeof(graph::VertexHeader);

					// vertex Id
					if constexpr (std::is_fundamental<VertexID>::value) {
						std::memcpy(CurrentPtr, &Data.VertexID_, sizeof(VertexID));
						CurrentPtr += sizeof(VertexID);
						Size += sizeof(VertexID);
					}
					else {
						Id.Serialize(CurrentPtr);
						CurrentPtr += Id.SerializerSize();
						Size += Id.SerializerSize();
					}

					// vertex Data
					if constexpr (std::is_fundamental<VertexData>::value) {
						std::memcpy(CurrentPtr, &Data.VertexValue_, sizeof(VertexData));
						CurrentPtr += sizeof(Data.VertexValue_);
						Size += sizeof(Data.VertexValue_);
					}
					/*else if (SerializableConcepts::is_string_like<VertexData>::value) {
						size_t str_len = Data.VertexValue_.size();
						std::memcpy(CurrentPtr, &str_len, sizeof(size_t));
						CurrentPtr += sizeof(size_t);
						Size += sizeof(size_t);

						std::memcpy(CurrentPtr, Data.VertexValue_.c_str(), str_len);
						CurrentPtr += str_len;
						Size += str_len;
					}*/
					else {
						auto Size = Data.VertexValue_.SerializerSize();
						std::vector<char> Res = Data.VertexValue_.Serialize();
						std::memcpy(CurrentPtr, Res.data(), Size);
						CurrentPtr += Data.VertexValue_.SerializerSize();
						Size += Data.VertexValue_.SerializerSize();
					}

					auto Offset = CurrentPtr - Buffer.data();
					InternalIdToOffsetMapping[Id] = std::make_pair(Offset, Size);
				}

				for (const auto& [Id, EdgeSet] : Edges) {
					for (const auto& Edge_ : EdgeSet) {
						graph::EdgeHeader EHeader{};
						EHeader.EdgeNumber = Edge_.InternalID;
						EHeader.SourceVertexIdOffset = sizeof(graph::EdgeHeader);
						EHeader.DestinationVertexIdOffset = EHeader.SourceVertexIdOffset + sizeof(Edge_.SourceInternalVertexID);
						EHeader.WeightOffset = EHeader.DestinationVertexIdOffset + sizeof(Edge_.DestinationInternalVertexID);

						std::memcpy(CurrentPtr, &EHeader, sizeof(graph::EdgeHeader));
						CurrentPtr += sizeof(graph::EdgeHeader);

						// source vertex Id

						std::memcpy(CurrentPtr, &Edge_.SourceInternalVertexID, sizeof(Edge_.SourceInternalVertexID));
						CurrentPtr += sizeof(Edge_.SourceInternalVertexID);

						// destination vertex Id
						std::memcpy(CurrentPtr, &Edge_.DestinationInternalVertexID, sizeof(Edge_.DestinationInternalVertexID));
						CurrentPtr += sizeof(Edge_.DestinationInternalVertexID);

						// weight
						if constexpr (std::is_fundamental<WeightType>::value) {
							std::memcpy(CurrentPtr, &Edge_.weight, sizeof(WeightType));
							CurrentPtr += sizeof(WeightType);
						}
						else {
							Edge_.weight.Serialize(CurrentPtr);
							CurrentPtr += Edge_.weight.SerializerSize();
						}
					}
				}

				ptrdiff_t VertexIdToInternalIdMappingOffset = CurrentPtr - HeaderPtr;
				GraphHeader_.VertexIdToInternalIdMappingOffset = VertexIdToInternalIdMappingOffset;
				std::memcpy(HeaderPtr, &GraphHeader_, sizeof(graph::GraphHeader));

				// Handle VertexID to InternalID Mapping
				auto IdMappingHeaderPtr = CurrentPtr;

				auto CurrentMappingSize = 0;
				for (const auto& [Id, InternalId] : VertexToInternalIdMapping) {
					if constexpr (std::is_fundamental<VertexID>::value) {
						std::memcpy(CurrentPtr, &Id, sizeof(VertexID));
						CurrentPtr += sizeof(VertexID);
						CurrentMappingSize += sizeof(VertexID);
					}
					else {
						Id.Serialize(CurrentPtr);
						CurrentPtr += Id.SerializerSize();
						CurrentMappingSize += Id.SerializerSize();
					}

					std::memcpy(CurrentPtr, &InternalId, sizeof(int));
					CurrentPtr += sizeof(int);
					CurrentMappingSize += sizeof(int);

					auto [Offset, Size] = InternalIdToOffsetMapping[Id];
					std::memcpy(CurrentPtr, &Offset, sizeof(size_t));
					CurrentPtr += sizeof(size_t);
					CurrentMappingSize += sizeof(size_t);

					std::memcpy(CurrentPtr, &Size, sizeof(size_t));
					CurrentPtr += sizeof(size_t);
					CurrentMappingSize += sizeof(size_t);
				}

				graph::IdMappingHeader IdMappingHeader_;
				IdMappingHeader_.MappingSize = CurrentMappingSize;
				std::memcpy(IdMappingHeaderPtr, &IdMappingHeader_, sizeof(graph::IdMappingHeader));
				CurrentPtr += sizeof(graph::IdMappingHeader);

				return Buffer;
			}

			void Deserialize(const char* Buffer) override {
				const char* CurrentPtr = Buffer;

				graph::GraphHeader Header;
				std::memcpy(&Header, CurrentPtr, sizeof(graph::GraphHeader));
				CurrentPtr += sizeof(graph::GraphHeader);

				InternalIdToVertexTypeMapping.clear();
				Edges.clear();
				VertexToInternalIdMapping.clear();

				for (auto i = 0; i < Header.VertexCount; ++i) {
					// Read vertex header
					graph::VertexHeader VHeader;
					std::memcpy(&VHeader, CurrentPtr, sizeof(graph::VertexHeader));
					CurrentPtr += sizeof(graph::VertexHeader);

					// Step 3: Deserialize the Vertex ID
					VertexID Id;
					if constexpr (std::is_fundamental<VertexID>::value) {
						std::memcpy(&Id, CurrentPtr, sizeof(VertexID));
						CurrentPtr += sizeof(VertexID);
					}
					else {
						Id.Deserialize(CurrentPtr);
						CurrentPtr += Id.SerializerSize();
					}

					// Step 4: Deserialize the Vertex Data
					VertexData Data;
					if constexpr (std::is_fundamental<VertexData>::value) {
						std::memcpy(&Data, CurrentPtr, sizeof(VertexData));
						CurrentPtr += sizeof(VertexData);
					}
					/*else if (SerializableConcepts::is_string_like<VertexData>::value) {
						size_t str_len = 0;
						std::memcpy(&str_len, CurrentPtr, sizeof(size_t));
						CurrentPtr += sizeof(size_t);
						std::memcpy(&Data, CurrentPtr, str_len);
						CurrentPtr += str_len;
					}*/
					else {
						Data.Deserialize(CurrentPtr);
						CurrentPtr += Data.SerializerSize();
					}

					auto InternalId = VHeader.VertexNumber;
					VertexType Vertex_(InternalId, Id, Data);
					InternalIdToVertexTypeMapping[InternalId] = Vertex_;
					VertexToInternalIdMapping[Id] = InternalId;
				}

				for (int i = 0; i < Header.EdgeCount; i++) {
					graph::EdgeHeader EHeader;
					std::memcpy(&EHeader, CurrentPtr, sizeof(graph::EdgeHeader));
					CurrentPtr += sizeof(graph::EdgeHeader);

					int SourceVertexInternalId{ 0 };
					std::memcpy(&SourceVertexInternalId, CurrentPtr, sizeof(int));
					CurrentPtr += sizeof(int);

					int DestinationVertexInternalId{ 0 };
					std::memcpy(&DestinationVertexInternalId, CurrentPtr, sizeof(int));
					CurrentPtr += sizeof(int);

					// Weight
					WeightType Weight;
					if constexpr (std::is_fundamental<WeightType>::value) {
						std::memcpy(&Weight, CurrentPtr, sizeof(WeightType));
						CurrentPtr += sizeof(WeightType);
					}
					else {
						Weight.Deserialize(CurrentPtr);
						CurrentPtr += Weight.SerializerSize();
					}

					auto EdgeInternalId = EHeader.EdgeNumber;
					EdgeType Edge_(EdgeInternalId, SourceVertexInternalId, DestinationVertexInternalId, Weight);
					AddEdge(Edge_);
				}
			}

			VertexData Get(const VertexID& Id) {
				if (VertexToInternalIdMapping.find(Id) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("vertex does not exist.");
				}
				VertexType& Vertex = InternalIdToVertexTypeMapping[VertexToInternalIdMapping[Id]];
				return Vertex.VertexValue_;
			}

			void AddNode(const VertexID& Id, const VertexData& Value) {
				if (VertexToInternalIdMapping.find(Id) != VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("Vertex already exists.");
				}
				VertexToInternalIdMapping[Id] = VertexInternalIDIterator;
				InternalIdToVertexTypeMapping[VertexInternalIDIterator] = VertexType(
					VertexInternalIDIterator, Id, Value);
				VertexInternalIDIterator++;
			}

			void AddEdge(const VertexID& FromVertexId, const VertexID& ToVertexId,
				WeightType Weight) {
				if (VertexToInternalIdMapping.find(FromVertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("FromVertexId does not exist.");
				}
				if (VertexToInternalIdMapping.find(ToVertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("ToVertexId does not exist.");
				}
				int FromVertexInternalId = VertexToInternalIdMapping[FromVertexId];
				int ToVertexInternalId = VertexToInternalIdMapping[ToVertexId];
				EdgeType Edge_(EdgeInternalIDIterator, FromVertexInternalId, ToVertexInternalId, Weight);
				AddEdge(Edge_);
			}

			void UpdateWeight(const VertexID& FromVertexId, const VertexID& ToVertexId,
				WeightType NewWeight) {
				if (InternalIdToVertexTypeMapping.find(FromVertexId) == InternalIdToVertexTypeMapping.end()) {
					throw std::invalid_argument("FromVertexId does not exist.");
				}
				if (InternalIdToVertexTypeMapping.find(ToVertexId) == InternalIdToVertexTypeMapping.end()) {
					throw std::invalid_argument("ToVertexId does not exist.");
				}
				int FromVertexInternalId = VertexToInternalIdMapping[FromVertexId];
				int ToVertexInternalId = VertexToInternalIdMapping[ToVertexId];
				auto& EdgeSet = Edges[FromVertexInternalId];
				for (auto it = EdgeSet.begin(); it != EdgeSet.end(); ++it) {
					if (it->DestinationInternalVertexID == ToVertexInternalId) {
						EdgeType ModifiedEdge = *it;
						EdgeSet.erase(it);

						ModifiedEdge.weight = NewWeight;

						EdgeSet.insert(ModifiedEdge);
						return;
					}
				}
				throw std::invalid_argument("Edge does not exist.");
			}

			WeightType GetEdgeWeight(const VertexID& FromVertexID, const VertexID& ToVertexID) {
				if (VertexToInternalIdMapping.find(FromVertexID) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("FromVertexId does not exist.");
				}
				int FromVertexInternalID = VertexToInternalIdMapping[FromVertexID];
				int ToVertexInternalID = VertexToInternalIdMapping[ToVertexID];

				auto& EdgeSet = Edges[FromVertexInternalID];
				for (const auto& Edge_ : EdgeSet) {
					if (Edge_.DestinationInternalVertexID == ToVertexInternalID) {
						return Edge_.weight;
					}
				}
				throw std::invalid_argument("Edge does not exist.");
			}

			std::vector<std::pair<VertexID, VertexData>>
				GetOutBoundNodes(const VertexID& VertexId) {
				if (VertexToInternalIdMapping.find(VertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("Vertex does not exist.");
				}
				int VertexInternalId = VertexToInternalIdMapping[VertexId];
				auto EdgesAtVertex = Edges[VertexInternalId];
				std::vector<std::pair<VertexID, VertexData>> OutBoundNodes;
				for (const auto& Edge_ : EdgesAtVertex) {
					auto& VertexTypeData = InternalIdToVertexTypeMapping[Edge_.DestinationInternalVertexID];
					OutBoundNodes.push_back(std::make_pair(
						VertexTypeData.VertexID_, VertexTypeData.VertexValue_));
				}
				return OutBoundNodes;
			}

			std::vector<std::pair<VertexID, VertexData>>
				GetInBoundNodes(const VertexID& VertexId) {
				if (VertexToInternalIdMapping.find(VertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("Vertex does not exist.");
				}
				int VertexInternalId = VertexToInternalIdMapping[VertexId];
				std::vector<std::pair<VertexID, VertexData>> InBoundNodes;
				for (const auto& [Source, EdgeSet] : Edges) {
					for (const auto& Edge : EdgeSet) {
						if (Edge.DestinationInternalVertexID == VertexInternalId) {
							auto& VertexTypeData = InternalIdToVertexTypeMapping[Edge.SourceInternalVertexID];
							InBoundNodes.push_back(
								std::make_pair(VertexTypeData.VertexID_, VertexTypeData.VertexValue_));
						}
					}
				}
				return InBoundNodes;
			}

			//// TODO(AnishDeshmukh1999): handle orphan graphs
			//void DeleteEdge(const VertexID& FromVertexId, const VertexID& ToVertexId) {
			//	if (InternalIdToVertexTypeMapping.find(FromVertexId) == InternalIdToVertexTypeMapping.end()) {
			//		throw std::invalid_argument("FromVertexId does not exist.");
			//	}
			//	if (InternalIdToVertexTypeMapping.find(ToVertexId) == InternalIdToVertexTypeMapping.end()) {
			//		throw std::invalid_argument("ToVertexId does not exist.");
			//	}
			//	auto EdgesAtVertex = Edges[FromVertexId];
			//	for (const auto& Edge : EdgesAtVertex) {
			//		if (Edge.destination == InternalIdToVertexTypeMapping[ToVertexId]) {
			//			EdgesAtVertex.erase(Edge);
			//			EdgeCount--;
			//			return;
			//		}
			//	}
			//	throw std::invalid_argument("Edge does not exist.");
			//}

			//// TODO(AnishDeshmukh1999): handle orphan graphs
			//void DeleteNode(const VertexID& VertexId) {
			//	InternalIdToVertexTypeMapping.erase(VertexId);
			//	Edges.erase(VertexId);
			//	for (auto& [Source, EdgeSet] : Edges) {
			//		std::vector<Edge> ToDelete;
			//		for (const auto& Edge : EdgeSet) {
			//			if (Edge.destination == VertexId) {
			//				ToDelete.push_back(Edge);
			//			}
			//		}
			//		for (const auto& Edge : ToDelete) {
			//			EdgeSet.erase(Edge);
			//		}
			//	}
			//}

			//void PrintGraphDFS(VertexID StartVertexId) {
			//	std::stack<VertexID> Stack;
			//	std::unordered_set<VertexID> Visited;
			//	Stack.push(StartVertexId);

			//	while (!Stack.empty()) {
			//		auto CurrentVertexId = Stack.top();
			//		Stack.pop();

			//		if (Visited.find(CurrentVertexId) == Visited.end()) {
			//			Visited.insert(CurrentVertexId);
			//			std::cout << CurrentVertexId << " ";

			//			for (const auto& Edge : Edges[CurrentVertexId]) {
			//				if (Visited.find(Edge.destination) == Visited.end()) {
			//					Stack.push(Edge.destination);
			//				}
			//			}
			//		}
			//	}
			//}

			//WeightType GetEdgeWeights(const VertexID& FromVertexId,
			//	const VertexID& ToVertexId) {
			//	if (InternalIdToVertexTypeMapping.find(FromVertexId) == InternalIdToVertexTypeMapping.end()) {
			//		throw std::invalid_argument("FromVertexId does not exist.");
			//	}
			//	if (InternalIdToVertexTypeMapping.find(ToVertexId) == InternalIdToVertexTypeMapping.end()) {
			//		throw std::invalid_argument("ToVertexId does not exist.");
			//	}
			//	auto& EdgeSet = Edges[FromVertexId];
			//	for (const auto& Edge : EdgeSet) {
			//		if (Edge.destination == ToVertexId) {
			//			return Edge.weight;
			//		}
			//	}
			//	throw std::invalid_argument("Edge does not exist.");
			//}
	};

	template <class VertexID, class VertexData, typename WeightType>
		requires graph::Hashable<VertexID>&& graph::Hashable<WeightType>&&
	SerializableConcepts::Serializable<VertexData>&&
		SerializableConcepts::Serializable<VertexID>&&
		SerializableConcepts::Serializable<WeightType>
		class Graph {
		private:
			std::unordered_map<std::string, SingleGraph<VertexID, VertexData, WeightType>>
				Graphs;

		public:
			using VertexDataPtr = VertexData;
			using GraphType = SingleGraph<VertexID, VertexData, WeightType>;

			Graph() = default;

			void AddNode(const std::string& Namespace, const VertexID& VertexId,
				const VertexData& Vertex) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
				}
				Graphs[Namespace].AddNode(VertexId, Vertex);
			}

			void AddEdge(const std::string& Namespace, const VertexID& FromVertexId,
				const VertexID& ToVertexId, WeightType Weight) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				Graphs[Namespace].AddEdge(FromVertexId, ToVertexId, Weight);
			}

			VertexData Get(const std::string& Namespace, const VertexID& Id) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].Get(Id);
			}

			WeightType GetEdgeWeight(const std::string& Namespace,
				const VertexID& FromVertexId,
				const VertexID& ToVertexId) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].GetEdgeWeight(FromVertexId, ToVertexId);
			}

			void UpdateWeight(const std::string& Namespace, const VertexID& FromVertexId,
				const VertexID& ToVertexId, WeightType NewWeight) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				Graphs[Namespace].UpdateWeight(FromVertexId, ToVertexId, NewWeight);
			}

			std::vector<std::pair<VertexID, VertexDataPtr>>
				GetOutBoundNodes(const std::string& Namespace, const VertexID& FromVertexId) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].GetOutBoundNodes(FromVertexId);
			}

			std::vector<std::pair<VertexID, VertexData>>
				GetInBoundNodes(const std::string& Namespace, const VertexID& ToVertexId) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].GetInBoundNodes(ToVertexId);
			}

			/*void DeleteEdge(const std::string& Namespace, const VertexID& FromVertexId,
				const VertexID& ToVertexId) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				Graphs[Namespace].DeleteEdge(FromVertexId, ToVertexId);
			}

			void PrintGraphDFS(const std::string& Namespace,
				const VertexID& StartVertex) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				Graphs[Namespace].PrintGraphDFS(StartVertex);
			}

			size_t NodeCount(const std::string& Namespace) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].InternalIdToVertexTypeMapping.SerializerSize();
			}

			size_t size(const std::string& Namespace) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].SerializerSize();
			}*/

			/*void Deserialize(const std::string& Namespace, const char* Buffer) {
				Graphs[Namespace].Deserialize(Buffer);
			}*/

			void Deserialize(const std::string& Namespace, const char* Buffer) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
				}
				Graphs[Namespace].Deserialize(Buffer);
			}

			std::vector<char> Serialize(const std::string& Namespace) {
				if (Graphs.find(Namespace) == Graphs.end()) {
					throw std::invalid_argument("Namespace does not exist.");
				}
				return Graphs[Namespace].Serialize();
			}
	};
}; // namespace FerryDB
#endif // INCLUDE_GRAPH_GRAPH_H_