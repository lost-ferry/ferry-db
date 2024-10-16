#pragma once
#include<concepts>
#include<optional>
#include <ostream>

namespace FerryDB {
	namespace graph {

		template<typename T>
		concept Printable = requires(T a, std::ostream & os) {
			{ os << a } -> std::same_as<std::ostream&>;
		};

		template<typename WeightType, typename VertexValue>
		concept PrintableGraphTypes = Printable<WeightType> && Printable<VertexValue>;


		template<typename WeightType>
		concept Weight = std::is_arithmetic_v<WeightType> || requires(WeightType w1, WeightType w2) {
			{ w1 < w2 } -> std::convertible_to<bool>;
			{ w1 > w2 } -> std::convertible_to<bool>;
			{ w1 == w2 } -> std::convertible_to<bool>;
		};

		template<typename VertexID, typename WeightType = int>
			requires Weight<WeightType>
		struct edge_descriptor {
			VertexID source;
			VertexID destination;
			WeightType weight;

			edge_descriptor(VertexID source, VertexID destination, WeightType weight = WeightType(1))
				: source(source), destination(destination), weight(weight) {}
		};

		template<typename T>
		concept Hashable = requires(T value) {
			{ std::hash<T>{}(value) } -> std::same_as<size_t>;
		};
	};
};

namespace std {
	template<typename VertexID, typename WeightType>
		requires FerryDB::graph::Hashable<VertexID>&& FerryDB::graph::Hashable<WeightType>
	struct hash<FerryDB::graph::edge_descriptor<VertexID, WeightType>> {
		size_t operator()(const FerryDB::graph::edge_descriptor<VertexID, WeightType>& ed) const {
			// Combine the hashes of source, destination, and weight
			// TODO : Find a better way to combine hashes
			return std::hash<VertexID>()(ed.source) ^
				(std::hash<VertexID>()(ed.destination) << 1) ^
				(std::hash<WeightType>()(ed.weight) << 2);
		}
	};

	template<typename VertexID, typename WeightType>
		requires FerryDB::graph::Hashable<VertexID>&& FerryDB::graph::Hashable<WeightType>
	struct equal_to<FerryDB::graph::edge_descriptor<VertexID, WeightType>> {
		bool operator()(const FerryDB::graph::edge_descriptor<VertexID, WeightType>& lhs,
			const FerryDB::graph::edge_descriptor<VertexID, WeightType>& rhs) const {
			return lhs.source == rhs.source &&
				lhs.destination == rhs.destination &&
				lhs.weight == rhs.weight;
		}
	};
}