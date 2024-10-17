#ifndef INCLUDE_CORE_NAMESPACE_H_
#define INCLUDE_CORE_NAMESPACE_H_
#include <cstddef>
#include <core/serializable.h>
namespace FerryDB{
    // Writer/Reader class
    class ObjectManager{
    public:
        void Save(Serializable::Serializable& s){
            char* data;
            std::size_t data_len;
            ReserveMemory(data, data_len, s);
            s.Serialize(data);
            // TODO save to mmap file
            delete[] data;
        }

        void Load(Serializable::Serializable& s){
            char* data;
            std::size_t data_len;
            ReserveMemory( data, data_len, s );
            // TODO load data_len from file
            s.Deserialize(data);
            delete[] data;
        }
    private:
        void ReserveMemory(char*& data, size_t& data_len, const Serializable::Serializable& s) {
            data_len = s.size();
            data = new char[data_len];
        }
    };
}

#endif // INCLUDE_CORE_NAMESPACE_H_