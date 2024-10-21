#ifndef INCLUDE_CORE_NAMESPACE_H_
#define INCLUDE_CORE_NAMESPACE_H_
#include <cstddef>
#include <core/serializable.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp> 
namespace FerryDB {

	// Writer/Reader class
	// This should go under the namespace
	template<typename T>
	class ObjectManager {
	private:
		std::string FileName;
	public:

		ObjectManager(const std::string& FileName) : FileName(FileName) {
			boost::interprocess::shared_memory_object::remove(FileName.c_str());
		}

		~ObjectManager() {
			using namespace boost::interprocess;
			shared_memory_object::remove(FileName.c_str());
		}

		void Save(SerializableConcepts::SerializableClass<T>& SObj) {
			using namespace boost::interprocess;

			shared_memory_object shm(create_only, FileName.c_str(), read_write);

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

		T Load() {
			using namespace boost::interprocess;

			shared_memory_object shm(open_only, FileName.c_str(), read_only);

			mapped_region region(shm, read_only);

			char* MappedData = static_cast<char*>(region.get_address());
			size_t MappedSize = region.get_size();

			Serializable::SerializedData LoadedData(MappedData, MappedSize);

			std::variant<T, Serializable::SerializableError> DeserializedObject = T::Deserialize(LoadedData);

			if (std::holds_alternative<T>(DeserializedObject)) {
				T SObj = std::get<T>(DeserializedObject);
				std::cout << "Object successfully deserialized from shared memory." << std::endl;
				return SObj;
			}
			else {
				// Handle deserialization error
				auto Error = std::get<Serializable::SerializableError>(DeserializedObject);
				std::cerr << "Deserialization error occurred: " << Error.what() << std::endl;
				throw Error;
			}
		}
	};
}

#endif // INCLUDE_CORE_NAMESPACE_H_