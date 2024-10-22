#ifndef INCLUDE_CORE_NAMESPACE_H_
#define INCLUDE_CORE_NAMESPACE_H_
#include <cstddef>
#include <string>
#include <core/serializable.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
namespace FerryDB {

	const std::string FILE_PATH = "E:\\Work\\ferrydb.frdb";

	// Writer/Reader class
	// This should go under the namespace
	template<typename T>
	class ObjectManager {
	private:
		std::string FileName;
	public:

		ObjectManager(const std::string& FileName) : FileName(FileName) {
		}

		ObjectManager() : FileName(FILE_PATH) {
		}

		~ObjectManager() {
		}

		void Save(SerializableConcepts::SerializableClass<T>& SObj) {
			using namespace boost::interprocess;

			size_t Size = SObj.SerializerSize();

			file_mapping MFile(FileName.c_str(), read_write);

			// TODO change mapped region to region of interest
			mapped_region Region(MFile, read_write, 0, Size);

			std::variant<Serializable::SerializedData, Serializable::SerializableError> SerializedData_ = T::Serialize(dynamic_cast<T&>(SObj));

			if (std::holds_alternative<Serializable::SerializedData>(SerializedData_)) {
				auto SerializedDataValue = std::get<Serializable::SerializedData>(SerializedData_);
				std::memcpy(Region.get_address(), SerializedDataValue.GetDataPtr(), Region.get_size());
			}
			else {
				auto Error = std::get<Serializable::SerializableError>(SerializedData_);
				std::cerr << "Serialization error occurred: " << Error.what() << std::endl;
				throw Error;
			}
		}

		T Load() {
			using namespace boost::interprocess;

			file_mapping MFile(FileName.c_str(), read_only);

			mapped_region Region(MFile, read_only);

			char* MappedData = static_cast<char*>(Region.get_address());
			size_t MappedSize = Region.get_size();

			//Serializable::SerializedData LoadedData(MappedData, MappedSize);

			std::variant<T, Serializable::SerializableError> DeserializedObject = T::Deserialize({ MappedData, MappedSize });

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