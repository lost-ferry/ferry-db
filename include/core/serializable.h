#ifndef INCLUDE_CORE_SERIALIZABLE_H_
#define INCLUDE_CORE_SERIALIZABLE_H_

#include <concepts>
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>

namespace FerryDB {

	namespace SerializerTags {
		enum MagicNumber
		{
			// "WGRH" in HEX
			WEIGHTED_GRAPH = 0x57475248,
		};
	};

	namespace SerializableConcepts {

		template <typename T>
		using is_string_like = std::is_same<std::decay_t<T>, std::string>;

		template <typename T>
		concept Serializable = std::is_fundamental<T>::value ||
			is_string_like<T>::value || requires(T a, const char* buffer) {
				{ a.SerializerSize() } -> std::same_as<std::size_t>;
				{ a.Serialize() } -> std::same_as<std::vector<char>>;
				{ a.Deserialize(buffer) } -> std::same_as<void>;
		};
	} // namespace SerializableConcepts

	namespace Serializable {
		class Serializable {
		public:
			virtual std::size_t SerializerSize() const = 0;
			virtual std::vector<char> Serialize() = 0;
			virtual void Deserialize(const char*) = 0;
		};
	}; // namespace Serializable
} // namespace FerryDB

#endif // INCLUDE_CORE_SERIALIZABLE_H_