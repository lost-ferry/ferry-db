#ifndef INCLUDE_CORE_SERIALIZABLE_H_
#define INCLUDE_CORE_SERIALIZABLE_H_

#include <concepts>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>
#include <variant>

namespace FerryDB {

	namespace SerializerTags {
		enum MagicNumber
		{
			// "WGRH" in HEX
			WEIGHTED_GRAPH = 0x57475248,
		};
	};

	namespace Serializable {

		class SerializedData {
		private:
			char* Data;
			std::size_t Length;

		public:
			SerializedData(std::size_t Length)
				: Data(new char[Length]), Length(Length) {
				// Optionally, initialize the allocated memory to zero
				std::memset(Data, 0, Length);
			}

			~SerializedData() {
				delete[] Data;
			}

			SerializedData(const SerializedData& other)
				: Data(new char[other.Length]), Length(other.Length) {
				std::memcpy(Data, other.Data, Length);
			}

			SerializedData& operator=(const SerializedData& other) {
				if (this == &other) {
					return *this;
				}
				delete[] Data;
				Length = other.Length;
				Data = new char[Length];
				std::memcpy(Data, other.Data, Length);
				return *this;
			}

			SerializedData(SerializedData&& other) noexcept
				: Data(other.Data), Length(other.Length) {
				other.Data = nullptr;
				other.Length = 0;
			}

			const char* GetData() const {
				return Data;
			}

			const char* GetDataPtr() const {
				return Data;
			}

			char* GetDataPtr() {
				return Data;
			}

			// Accessor for length
			std::size_t GetLength() const {
				return Length;
			}
		};

		enum SerializableErrors {
			NO_ERROR = 0,
			NO_SERIALIZABLE_DATA = 1,
			NO_DESERIALIZABLE_DATA = 2,
			SERIALIZABLE_DATA_CORRUPTED = 3,
			DESERIALIZABLE_DATA_CORRUPTED = 4,
			NO_NAMESPACE = 5,
		};

		class SerializableError : public std::exception {
		private:
			SerializableErrors ErrorCode_;
			std::string ErrorMessage_;

		public:
			// Constructor
			SerializableError(const SerializableErrors& Code, const std::string& Message)
				: ErrorCode_(Code), ErrorMessage_(Message) {}

			// Accessor for error code
			SerializableErrors getErrorCode() const noexcept {
				return ErrorCode_;
			}

			// Accessor for error message
			const char* what() const noexcept override {
				return ErrorMessage_.c_str();
			}
		};

	}; // namespace Serializable

	namespace SerializableConcepts {

		template <typename T>
		using is_string_like = std::is_same<std::decay_t<T>, std::string>;

		template <typename T>
		concept Serializable = std::is_fundamental<T>::value ||
			is_string_like<T>::value || requires(T a, const Serializable::SerializedData & buffer) {
				{ a.SerializerSize() } -> std::same_as<std::size_t>;
				{ a.Serialize() } -> std::same_as<std::variant<Serializable::SerializedData, Serializable::SerializableError>>;
				{ a.Deserialize(buffer) } -> std::same_as<std::variant<void, Serializable::SerializableError>>;
		};
	} // namespace SerializableConcepts

} // namespace FerryDB

#endif // INCLUDE_CORE_SERIALIZABLE_H_