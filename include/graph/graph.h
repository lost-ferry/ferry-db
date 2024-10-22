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
		class SingleGraph : public SerializableConcepts::SerializableClass<SingleGraph<VertexID, VertexData, WeightType>> {
		public:
			using EdgeType = graph::edge_descriptor<WeightType>;
			using VertexType = graph::vertex_descriptor<VertexID, VertexData>;

		private:
			std::unordered_map<VertexID, size_t> VertexToInternalIdMapping;
			std::unordered_map<size_t, VertexType> InternalIdToVertexTypeMapping;
			std::unordered_map<size_t, std::unordered_set<EdgeType>> Edges;
			size_t VertexInternalIDIterator = 0;
			size_t EdgeInternalIDIterator = 0;

			void AddEdge(const EdgeType& Edge) {
				Edges[Edge.SourceInternalVertexID].insert(Edge);
				EdgeInternalIDIterator++;
			}

			size_t GetVertexDataSize(const VertexData& Data) const {
				if constexpr (std::is_fundamental<VertexData>::value) {
					return sizeof(VertexData);
				}
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

			size_t GetWeightTypeSize(const WeightType& Weight) const {
				if constexpr (std::is_fundamental<WeightType>::value) {
					return sizeof(WeightType);
				}
				else {
					return Weight.SerializerSize();
				}
			}

			size_t GetEdgeSize(const EdgeType& Edge) const {
				size_t result = sizeof(Edge.SourceInternalVertexID) +
					sizeof(Edge.DestinationInternalVertexID) +
					GetWeightTypeSize(Edge.weight);
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
				return HeaderSize + VertexIdDataSize + EdgeDataSize + IdMappingSize;
			}

			// Assume Buffer is of size SerializerSize()

			// return class with destructor to free memory

			static std::variant<FerryDB::Serializable::SerializedData, FerryDB::Serializable::SerializableError> Serialize(const SingleGraph<VertexID, VertexData, WeightType>& GraphToSerialize) {
				using SerializedData = FerryDB::Serializable::SerializedData;
				size_t Size = GraphToSerialize.SerializerSize();

				SerializedData ResponseSerializedData(Size);

				graph::GraphHeader GraphHeader_;
				GraphHeader_.MagicNumber = SerializerTags::MagicNumber::WEIGHTED_GRAPH;
				GraphHeader_.VertexCount = GraphToSerialize.InternalIdToVertexTypeMapping.size();
				GraphHeader_.EdgeCount = GraphToSerialize.EdgeInternalIDIterator;
				GraphHeader_.VertexStartOffset = sizeof(graph::GraphHeader);
				GraphHeader_.EdgeStartOffset = GraphHeader_.VertexStartOffset + GraphToSerialize.GetAllVertexSize();

				char* HeaderPtr = ResponseSerializedData.GetDataPtr();
				char* CurrentPtr = ResponseSerializedData.GetDataPtr() + sizeof(graph::GraphHeader);


				// Pair of Offset and Size
				std::unordered_map<size_t, std::pair<size_t, size_t>> InternalIdToOffsetMapping;

				for (const auto& [Id, Data] : GraphToSerialize.InternalIdToVertexTypeMapping) {
					size_t Size{ 0 };
					graph::VertexHeader VHeader{};
					VHeader.VertexNumber = Data.InternalID;
					// Relative offsets
					VHeader.VertexIdOffset =
						sizeof(graph::VertexHeader);
					VHeader.VertexDataOffset = VHeader.VertexIdOffset + GraphToSerialize.GetVertexIdSize(Data.VertexID_);
					VHeader.VertexDataSize = GraphToSerialize.GetVertexDataSize(Data.VertexValue_);
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
						std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedId = Data.VertexID_.Serialize();
						if (std::holds_alternative<Serializable::SerializableError>(SerializedId)) {
							return std::get<Serializable::SerializableError>(SerializedId);
						}
						size_t SerializerSize = Data.VertexID_.SerializerSize();
						std::memcpy(CurrentPtr, std::get<Serializable::SerializedData>(SerializedId).GetDataPtr(), SerializerSize);
						CurrentPtr += SerializerSize;
						Size += SerializerSize;
					}

					// vertex Data
					if constexpr (std::is_fundamental<VertexData>::value) {
						std::memcpy(CurrentPtr, &Data.VertexValue_, sizeof(VertexData));
						CurrentPtr += sizeof(Data.VertexValue_);
						Size += sizeof(Data.VertexValue_);
					}
					else {
						auto Size = Data.VertexValue_.SerializerSize();
						std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedVertexValue = Data.VertexValue_.Serialize();
						if (std::holds_alternative<Serializable::SerializableError>(SerializedVertexValue)) {
							return std::get<Serializable::SerializableError>(SerializedVertexValue);
						}
						std::memcpy(CurrentPtr, std::get<Serializable::SerializedData>(SerializedVertexValue).GetDataPtr(), Size);
						CurrentPtr += Data.VertexValue_.SerializerSize();
						Size += Data.VertexValue_.SerializerSize();
					}

					size_t Offset = CurrentPtr - ResponseSerializedData.GetDataPtr();
					InternalIdToOffsetMapping[Id] = std::make_pair(Offset, Size);
				}

				for (const auto& [Id, EdgeSet] : GraphToSerialize.Edges) {
					for (const auto& Edge_ : EdgeSet) {
						graph::EdgeHeader EHeader{};
						EHeader.EdgeNumber = Edge_.InternalID;
						EHeader.SourceVertexIdOffset = sizeof(graph::EdgeHeader);
						EHeader.DestinationVertexIdOffset = EHeader.SourceVertexIdOffset + sizeof(Edge_.SourceInternalVertexID);
						EHeader.WeightOffset = EHeader.DestinationVertexIdOffset + sizeof(Edge_.DestinationInternalVertexID);
						EHeader.WeightSize = GraphToSerialize.GetWeightTypeSize(Edge_.weight);

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
							size_t SerializerSize = Edge_.weight.SerializerSize();
							std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedWeight = Edge_.weight.Serialize();
							if (std::holds_alternative<Serializable::SerializableError>(SerializedWeight)) {
								return std::get<Serializable::SerializableError>(SerializedWeight);
							}
							std::memcpy(CurrentPtr, std::get<Serializable::SerializedData>(SerializedWeight).GetDataPtr(), SerializerSize);
							CurrentPtr += SerializerSize;
						}
					}
				}

				ptrdiff_t VertexIdToInternalIdMappingOffset = CurrentPtr - HeaderPtr;
				GraphHeader_.VertexIdToInternalIdMappingOffset = VertexIdToInternalIdMappingOffset;
				std::memcpy(HeaderPtr, &GraphHeader_, sizeof(graph::GraphHeader));

				// Handle VertexID to InternalID Mapping
				auto IdMappingHeaderPtr = CurrentPtr;
				CurrentPtr += sizeof(graph::IdMappingHeader);

				auto CurrentMappingSize = 0;
				for (const auto& [Id, InternalId] : GraphToSerialize.VertexToInternalIdMapping) {
					if constexpr (std::is_fundamental<VertexID>::value) {
						std::memcpy(CurrentPtr, &Id, sizeof(VertexID));
						CurrentPtr += sizeof(VertexID);
						CurrentMappingSize += sizeof(VertexID);
					}
					else {
						auto SerializerSize = Id.SerializerSize();
						std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedVertexIdData = Id.Serialize();
						if (std::holds_alternative<Serializable::SerializableError>(SerializedVertexIdData)) {
							return std::get<Serializable::SerializableError>(SerializedVertexIdData);
						}
						std::memcpy(CurrentPtr, std::get<Serializable::SerializedData>(SerializedVertexIdData).GetDataPtr(), SerializerSize);
						CurrentPtr += SerializerSize;
						CurrentMappingSize += SerializerSize;
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

				return ResponseSerializedData;
			}

			static std::variant<SingleGraph<VertexID, VertexData, WeightType>, FerryDB::Serializable::SerializableError> Deserialize(const FerryDB::Serializable::SerializedData& Buffer) {
				// TODO handle case where graph cannot be parsed from buffer
				const char* CurrentPtr = Buffer.GetDataPtr();

				SingleGraph<VertexID, VertexData, WeightType> ResultGraph{};

				graph::GraphHeader Header;
				std::memcpy(&Header, CurrentPtr, sizeof(graph::GraphHeader));
				CurrentPtr += sizeof(graph::GraphHeader);

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
						auto Diff = VHeader.VertexDataOffset - VHeader.VertexIdOffset;
						const Serializable::SerializedData IdBuffer(CurrentPtr, Diff);
						auto DeserializedRes = Id.Deserialize(IdBuffer);
						if (std::holds_alternative<Serializable::SerializableError>(DeserializedRes)) {
							return std::get<Serializable::SerializableError>(DeserializedRes);
						}
						CurrentPtr += Diff;
					}

					// Step 4: Deserialize the Vertex Data
					VertexData Data;
					if constexpr (std::is_fundamental<VertexData>::value) {
						std::memcpy(&Data, CurrentPtr, sizeof(VertexData));
						CurrentPtr += sizeof(VertexData);
					}
					else {
						auto Diff = VHeader.VertexDataSize;
						const Serializable::SerializedData DataBuffer(CurrentPtr, Diff);
						auto DeserializedRes = Data.Deserialize(DataBuffer);
						CurrentPtr += Diff;
					}

					auto InternalId = VHeader.VertexNumber;
					VertexType Vertex_(InternalId, Id, Data);
					ResultGraph.InternalIdToVertexTypeMapping[InternalId] = Vertex_;
					ResultGraph.VertexToInternalIdMapping[Id] = InternalId;
				}

				for (int i = 0; i < Header.EdgeCount; i++) {
					graph::EdgeHeader EHeader;
					std::memcpy(&EHeader, CurrentPtr, sizeof(graph::EdgeHeader));
					auto temp = sizeof(graph::EdgeHeader);
					CurrentPtr += sizeof(graph::EdgeHeader);

					size_t SourceVertexInternalId{ 0 };
					std::memcpy(&SourceVertexInternalId, CurrentPtr, sizeof(int));
					CurrentPtr += sizeof(int);

					size_t DestinationVertexInternalId{ 0 };
					std::memcpy(&DestinationVertexInternalId, CurrentPtr, sizeof(int));
					CurrentPtr += sizeof(int);

					// Weight
					WeightType Weight;
					if constexpr (std::is_fundamental<WeightType>::value) {
						std::memcpy(&Weight, CurrentPtr, sizeof(WeightType));
						CurrentPtr += sizeof(WeightType);
					}
					else {
						auto Diff = EHeader.WeightSize;
						const Serializable::SerializedData WeightBuffer(CurrentPtr, Diff);
						auto DeserializedWeight = Weight.Deserialize(WeightBuffer);
						if (std::holds_alternative<Serializable::SerializableError>(DeserializedWeight)) {
							return std::get<Serializable::SerializableError>(DeserializedWeight);
						}
						CurrentPtr += Diff;
					}

					auto EdgeInternalId = EHeader.EdgeNumber;
					EdgeType Edge_(EdgeInternalId, SourceVertexInternalId, DestinationVertexInternalId, Weight);
					ResultGraph.AddEdge(Edge_);
				}

				return ResultGraph;
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
				auto FromVertexInternalId = VertexToInternalIdMapping[FromVertexId];
				auto ToVertexInternalId = VertexToInternalIdMapping[ToVertexId];
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
				auto FromVertexInternalId = VertexToInternalIdMapping[FromVertexId];
				auto ToVertexInternalId = VertexToInternalIdMapping[ToVertexId];
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
				auto FromVertexInternalID = VertexToInternalIdMapping[FromVertexID];
				auto ToVertexInternalID = VertexToInternalIdMapping[ToVertexID];

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
				std::vector<std::pair<VertexID, VertexData>> OutBoundNodes;
				auto VertexInternalId = VertexToInternalIdMapping[VertexId];
				if (Edges.find(VertexInternalId) == Edges.end()) {
					return OutBoundNodes;
				}
				auto EdgesAtVertex = Edges[VertexInternalId];
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
				auto VertexInternalId = VertexToInternalIdMapping[VertexId];
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
			void DeleteEdge(const VertexID& FromVertexId, const VertexID& ToVertexId) {
				if (VertexToInternalIdMapping.find(FromVertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("FromVertexId does not exist.");
				}
				if (VertexToInternalIdMapping.find(ToVertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("ToVertexId does not exist.");
				}
				auto FromVertexInternalId = VertexToInternalIdMapping[FromVertexId];
				auto ToVertexInternalId = VertexToInternalIdMapping[ToVertexId];
				auto& EdgesAtVertex = Edges[FromVertexInternalId];

				for (auto It = EdgesAtVertex.begin(); It != EdgesAtVertex.end();) {
					if (It->DestinationInternalVertexID == ToVertexInternalId) {
						EdgesAtVertex.erase(It++);
						EdgeInternalIDIterator--;
					}
					else {
						It++;
					}
				}

				return;
			}

			//// TODO(AnishDeshmukh1999): handle orphan graphs
			void DeleteNode(const VertexID& VertexId) {
				if (VertexToInternalIdMapping.find(VertexId) == VertexToInternalIdMapping.end()) {
					throw std::invalid_argument("Vertex does not exist.");
				}
				auto VertexInternalId = VertexToInternalIdMapping[VertexId];
				InternalIdToVertexTypeMapping.erase(VertexInternalId);
				if (Edges.find(VertexInternalId) != Edges.end()) {
					auto EdgesAtVertex = Edges[VertexInternalId];
					EdgeInternalIDIterator -= EdgesAtVertex.size();
					Edges.erase(VertexInternalId);
				}
				VertexInternalIDIterator--;
				for (auto& [Source, EdgeSet] : Edges) {
					std::vector<EdgeType> ToDelete;
					for (const auto& Edge : EdgeSet) {
						if (Edge.DestinationInternalVertexID == VertexInternalId) {
							ToDelete.push_back(Edge);
						}
					}
					for (const auto& Edge : ToDelete) {
						EdgeSet.erase(Edge);
						EdgeInternalIDIterator--;
					}
				}
				for (auto It = Edges.begin(); It != Edges.end();) {
					if (Edges[It->first].empty()) {
						Edges.erase(It++);
					}
					else {
						It++;
					}
				}
				VertexToInternalIdMapping.erase(VertexId);
			}

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

	//template <class VertexID, class VertexData, typename WeightType>
	//	requires graph::Hashable<VertexID>&& graph::Hashable<WeightType>&&
	//SerializableConcepts::Serializable<VertexData>&&
	//	SerializableConcepts::Serializable<VertexID>&&
	//	SerializableConcepts::Serializable<WeightType>
	//	class Graph {
	//	private:
	//		std::unordered_map<std::string, SingleGraph<VertexID, VertexData, WeightType>>
	//			Graphs;

	//	public:
	//		using VertexDataPtr = VertexData;
	//		using GraphType = SingleGraph<VertexID, VertexData, WeightType>;

	//		template <typename T>
	//		using graph_variant_t<T> = std::variant<T, FerryDB::Serializable::SerializableError>;

	//		Graph() = default;

	//		void AddNode(const std::string& Namespace, const VertexID& VertexId,
	//			const VertexData& Vertex) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
	//			}
	//			Graphs[Namespace].AddNode(VertexId, Vertex);
	//		}

	//		void AddEdge(const std::string& Namespace, const VertexID& FromVertexId,
	//			const VertexID& ToVertexId, WeightType Weight) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			Graphs[Namespace].AddEdge(FromVertexId, ToVertexId, Weight);
	//		}

	//		VertexData Get(const std::string& Namespace, const VertexID& Id) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].Get(Id);
	//		}

	//		WeightType GetEdgeWeight(const std::string& Namespace,
	//			const VertexID& FromVertexId,
	//			const VertexID& ToVertexId) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].GetEdgeWeight(FromVertexId, ToVertexId);
	//		}

	//		void UpdateWeight(const std::string& Namespace, const VertexID& FromVertexId,
	//			const VertexID& ToVertexId, WeightType NewWeight) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			Graphs[Namespace].UpdateWeight(FromVertexId, ToVertexId, NewWeight);
	//		}

	//		std::vector<std::pair<VertexID, VertexDataPtr>>
	//			GetOutBoundNodes(const std::string& Namespace, const VertexID& FromVertexId) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].GetOutBoundNodes(FromVertexId);
	//		}

	//		std::vector<std::pair<VertexID, VertexData>>
	//			GetInBoundNodes(const std::string& Namespace, const VertexID& ToVertexId) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].GetInBoundNodes(ToVertexId);
	//		}

	//		/*void DeleteEdge(const std::string& Namespace, const VertexID& FromVertexId,
	//			const VertexID& ToVertexId) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			Graphs[Namespace].DeleteEdge(FromVertexId, ToVertexId);
	//		}

	//		void PrintGraphDFS(const std::string& Namespace,
	//			const VertexID& StartVertex) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			Graphs[Namespace].PrintGraphDFS(StartVertex);
	//		}

	//		size_t NodeCount(const std::string& Namespace) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].InternalIdToVertexTypeMapping.SerializerSize();
	//		}

	//		size_t size(const std::string& Namespace) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				throw std::invalid_argument("Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].SerializerSize();
	//		}*/

	//		/*void Deserialize(const std::string& Namespace, const char* Buffer) {
	//			Graphs[Namespace].Deserialize(Buffer);
	//		}*/

	//		template<typename VertexID, typename VertexData, typename WeightType = int>
	//		static graph_variant_t<FerryDB::Graph<>> Deserialize(const std::string& Namespace, const FerryDB::Serializable::SerializedData& SerializedData) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				Graphs[Namespace] = SingleGraph<VertexID, VertexData, WeightType>();
	//			}
	//			Graphs[Namespace].Deserialize(SerializedData);
	//		}

	//		static graph_variant_t<FerryDB::Serializable::SerializedData> Serialize(const std::string& Namespace) {
	//			if (Graphs.find(Namespace) == Graphs.end()) {
	//				return FerryDB::Serializable::SerializableError(Serializable::NO_NAMESPACE, "Namespace does not exist.");
	//			}
	//			return Graphs[Namespace].Serialize();
	//		}
	//};
}; // namespace FerryDB
#endif // INCLUDE_GRAPH_GRAPH_H_