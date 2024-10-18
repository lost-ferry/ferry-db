#pragma once
#include <concepts>
#include <optional>
#include <ostream>
#include <functional>

namespace FerryDB {
	namespace graph {

		template <typename T>
		concept Printable = requires(T a, std::ostream & os) {
			{ os << a } -> std::same_as<std::ostream&>;
		};

		template <typename WeightType, typename VertexData>
		concept PrintableGraphTypes = Printable<WeightType> && Printable<VertexData>;

		template <typename WeightType>
		concept Weight =
			std::is_arithmetic_v<WeightType> || requires(WeightType w1, WeightType w2) {
				{ w1 < w2 } -> std::convertible_to<bool>;
				{ w1 > w2 } -> std::convertible_to<bool>;
				{ w1 == w2 } -> std::convertible_to<bool>;
		};

		template <typename VertexID, typename VertexData>
		struct vertex_descriptor {
			int InternalID;
			VertexID VertexID_;
			VertexData VertexValue_;

			vertex_descriptor(int InternalID, VertexID VertexID_, VertexData VertexValue_)
				: InternalID(InternalID), VertexID_(VertexID_), VertexValue_(VertexValue_) {}

			vertex_descriptor() : InternalID(0), VertexID_(), VertexValue_() {}
		};

		template <typename WeightType = int>
			requires Weight<WeightType>
		struct edge_descriptor {
			int InternalID;
			int SourceInternalVertexID;
			int DestinationInternalVertexID;
			WeightType weight;

			edge_descriptor(int InternalID, int SourceInternalVertexID, int DestinationInternalVertexID,
				WeightType weight = WeightType(1))
				:InternalID(InternalID), SourceInternalVertexID(SourceInternalVertexID), DestinationInternalVertexID(DestinationInternalVertexID), weight(weight) {}
		};

		template <typename T>
		concept Hashable = requires(T value) {
			{ std::hash<T>{}(value) } -> std::same_as<size_t>;
		};

		struct GraphHeader {
			size_t MagicNumber{ 0 };
			size_t Version{ 1 };
			size_t VertexCount{ 0 };
			size_t VertexStartOffset{ 0 };
			size_t EdgeCount{ 0 };
			size_t EdgeStartOffset{ 0 };
			size_t VertexIdToInternalIdMappingOffset{ 0 };
		};

		struct VertexHeader {
			int VertexNumber{ 0 };
			size_t VertexIdOffset{ 0 };
			size_t VertexDataOffset{ 0 };
		};

		struct EdgeHeader {
			int EdgeNumber{ 0 };
			size_t SourceVertexIdOffset{ 0 };
			size_t DestinationVertexIdOffset{ 0 };
			size_t WeightOffset{ 0 };
		};

		struct IdMappingHeader {
			size_t MappingSize{ 0 };
		};
	};  // namespace graph
};  // namespace FerryDB

namespace std {

	template <typename VertexID, typename VertexData>
	struct hash<FerryDB::graph::vertex_descriptor<VertexID, VertexData>> {
		size_t operator()(
			const FerryDB::graph::vertex_descriptor<VertexID, VertexData>& vtx) const {
			return std::hash<int>()(vtx.InternalID);
		}
	};

	template <typename VertexID, typename VertexData>
	struct equal_to<FerryDB::graph::vertex_descriptor<VertexID, VertexData>> {
		bool operator()(
			const FerryDB::graph::vertex_descriptor<VertexID, VertexData>& lhs,
			const FerryDB::graph::vertex_descriptor<VertexID, VertexData>& rhs) const {
			return lhs.InternalID == rhs.InternalID;
		}
	};

	template <typename WeightType>
	struct hash<FerryDB::graph::edge_descriptor<WeightType>> {
		size_t operator()(
			const FerryDB::graph::edge_descriptor<WeightType>& ed) const {
			return std::hash<int>()(ed.InternalID);
		}
	};

	template <typename WeightType>
		requires FerryDB::graph::Hashable<WeightType>
	struct equal_to<FerryDB::graph::edge_descriptor<WeightType>> {
		bool operator()(
			const FerryDB::graph::edge_descriptor<WeightType>& lhs,
			const FerryDB::graph::edge_descriptor<WeightType>& rhs) const {
			return lhs.SourceInternalVertexID == rhs.SourceInternalVertexID && lhs.DestinationInternalVertexID == rhs.DestinationInternalVertexID;
		}
	};
}  // namespace std
