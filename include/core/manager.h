#ifndef INCLUDE_CORE_NAMESPACE_H_
#define INCLUDE_CORE_NAMESPACE_H_
#include <cstddef>
#include <core/serializable.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp> 
namespace FerryDB {

	// Writer/Reader class
	template<typename T>
	class ObjectManager {
	public:
		void Save(SerializableConcepts::SerializableClass<T>& SObj, const std::string& FileName) {
			using namespace boost::interprocess;

			shared_memory_object shm(create_only, "ObjectData", read_write);

			size_t Size = SObj.SerializerSize();
			// Resize the memory object to the given size
			shm.truncate(Size);

			// Map the shared memory into this process's address space
			mapped_region region(shm, read_write);
			std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedData_ = T::Serialize(dynamic_cast<T&>(SObj));

			if (std::holds_alternative<Serializable::SerializedData>(SerializedData_)) {
				auto SerializedDataValue = std::get<Serializable::SerializedData>(SerializedData_);
				std::memcpy(region.get_address(), SerializedDataValue.GetDataPtr(), region.get_size());
			}
			else {
				auto Error = std::get<Serializable::SerializableError>(SerializedData_);
				std::cerr << "Serialization error occurred: " << Error.what() << std::endl;
				throw Error;
			}
		}

		/*T Load(const std::string& FileName) {
			using namespace boost::interprocess;

			managed_mapped_file MappedFile(open_read_only, FileName.c_str());

			size_t* MappedLength = MappedFile.find<size_t>("ObjectSize").first;
			if (MappedLength == nullptr) {
				std::cerr << "Object size not found in the file" << std::endl;
				throw Serializable::SerializableError(Serializable::SerializableErrors::NO_DESERIALIZABLE_DATA, "Object size not found in the file");
			}

			size_t Size = *MappedLength;
			char* MappedData = MappedFile.find<char>("ObjectData").first;
			if (MappedData == nullptr) {
				throw std::runtime_error("Failed to retrieve the serialized data.");
			}

			Serializable::SerializedData SerializedData(MappedData, Size);

			std::variant<T, Serializable::SerializableError> DeserializedData = T::Deserialize(SerializedData);
			if (std::holds_alternative<T>(DeserializedData)) {
				auto SObj = std::get<T>(DeserializedData);
				return SObj;
			}
			else {
				auto Error = std::get<Serializable::SerializableError>(DeserializedData);
				std::cerr << "Deserialization error occurred: " << Error.what() << std::endl;
				throw Error;
			}
		}*/
	private:
		/*void ReserveMemory(char*& data, size_t& data_len, const Serializable::Serializable& s) {
			data_len = s.size();
			data = new char[data_len];
		}*/
	};
}

#endif // INCLUDE_CORE_NAMESPACE_H_