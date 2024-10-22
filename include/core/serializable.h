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
			bool OwnsData;

		public:
			SerializedData(std::size_t Length)
				: Data(new char[Length]), Length(Length), OwnsData(true) {
				// Optionally, initialize the allocated memory to zero
				std::memset(Data, 0, Length);
			}

			SerializedData(char* ExternalData, std::size_t Length)
				: Data(ExternalData), Length(Length), OwnsData(false) {}

			~SerializedData() {
				if (OwnsData) {
					delete[] Data;
				}
			}

			SerializedData(const SerializedData& Other)
				: Length(Other.Length), OwnsData(Other.OwnsData) {
				if (OwnsData) {
					Data = new char[Other.Length];
					std::memcpy(Data, Other.Data, Length);  // Deep copy of the data
				}
				else {
					Data = Other.Data;
				}
			}

			SerializedData& operator=(const SerializedData& Other) {
				if (this == &Other) {
					return *this;  // Guard against self-assignment
				}

				if (OwnsData) {
					delete[] Data;  // Free the existing data
					Data = new char[Other.Length];
					Length = Other.Length;
					OwnsData = true;  // This object now owns the new data
					std::memcpy(Data, Other.Data, Length);  // Deep copy of the data
				}
				else {
					Data = Other.Data;
					Length = Other.Length;
					OwnsData = false;
				}

				return *this;
			}

			SerializedData(SerializedData&& Other) noexcept
				: Data(Other.Data), Length(Other.Length), OwnsData(Other.OwnsData) {
				Other.Data = nullptr;  // Leave the moved-from object in a valid state
				Other.Length = 0;
				Other.OwnsData = false;
			}

			SerializedData& operator=(SerializedData&& Other) noexcept {
				if (this == &Other) {
					return *this;
				}

				if (OwnsData) {
					delete[] Data;  // Free the existing data
				}

				// Transfer ownership from the other object
				Data = Other.Data;
				Length = Other.Length;
				OwnsData = Other.OwnsData;

				// Leave the moved-from object in a valid state
				Other.Data = nullptr;
				Other.Length = 0;
				Other.OwnsData = false;

				return *this;
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
			SerializableErrors GetErrorCode() const noexcept {
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
			is_string_like<T>::value || requires(T A, const Serializable::SerializedData & Buffer) {
				{ A.SerializerSize() } -> std::same_as<std::size_t>;
				{ A.Serialize() } -> std::same_as<std::variant<Serializable::SerializedData, Serializable::SerializableError>>;
				{ A.Deserialize(Buffer) } -> std::same_as<std::variant<void, Serializable::SerializableError>>;
		};

		// CRTP Pattern
		template<typename T>
		class SerializableClass {
		public:
			static std::variant<Serializable::SerializedData, Serializable::SerializableError> Serialize() {
				return T::Serialize();
			};

			static std::variant<T, Serializable::SerializableError> Deserialize(const Serializable::SerializedData& Buffer) {
				return T::Deserialize(Buffer);
			};

			virtual size_t SerializerSize() const = 0;
		};
	};

} // namespace FerryDB

#endif // INCLUDE_CORE_SERIALIZABLE_H_