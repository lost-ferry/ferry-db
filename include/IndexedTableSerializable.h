#ifndef INDEXEDTABLE_SERIALIZABLE_H_
#define INDEXEDTABLE_SERIALIZABLE_H_

#include <concepts>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <type_traits>
#include <variant>

namespace FerryDB {

	namespace IndexedTableSerializerTags {
		enum MagicNumber {
			INDEXED_TABLE = 0x49445442,
		};
	};

	namespace IndexedTableSerializable {

		class SerializedData {
		private:
			char* Data;
			std::size_t Length;

		public:
			SerializedData(std::size_t Length)
				: Data(new char[Length]), Length(Length) {
				std::memset(Data, 0, Length);
			}

			~SerializedData() {
				delete[] Data;
			}

			SerializedData(const char* buffer, std::size_t length)
                : Data(new char[length]), Length(length) {
                std::memcpy(Data, buffer, length);
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

			// Keep this as-is, since it is non-const
            char* GetDataPtr() {
                return Data;
            }

            // Make this method const, so it can be used with const objects
            const char* GetDataPtr() const {
                return Data;
            }

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
		};

		class SerializableError : public std::exception {
		private:
			SerializableErrors ErrorCode_;
			std::string ErrorMessage_;

		public:
			SerializableError(const SerializableErrors& Code, const std::string& Message)
				: ErrorCode_(Code), ErrorMessage_(Message) {}

			SerializableErrors getErrorCode() const noexcept {
				return ErrorCode_;
			}

			const char* what() const noexcept override {
				return ErrorMessage_.c_str();
			}
		};
	};

	namespace IndexedTableSerializableConcepts {

		template <typename T>
		concept is_string_like = std::is_same<std::decay_t<T>, std::string>::value;

		template <typename T>
		concept Serializable = (std::is_fundamental_v<T> || is_string_like<T>) ||
			requires(T a, const IndexedTableSerializable::SerializedData& buffer) {
				{ a.SerializerSize() } -> std::same_as<std::size_t>;
				{ a.Serialize() } -> std::same_as<std::variant<IndexedTableSerializable::SerializedData, IndexedTableSerializable::SerializableError>>;
				{ a.Deserialize(buffer) } -> std::same_as<std::variant<void, IndexedTableSerializable::SerializableError>>;
		};

		// CRTP Pattern for Serializable Class
		template<typename T>
		class SerializableClass {
		public:
			static std::variant<IndexedTableSerializable::SerializedData, IndexedTableSerializable::SerializableError> Serialize() {
				return T::Serialize();
			}

			static std::variant<T, IndexedTableSerializable::SerializableError> Deserialize(const IndexedTableSerializable::SerializedData& buffer) {
				return T::Deserialize(buffer);
			}
		};
	};

} 

#endif 
