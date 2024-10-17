#ifndef INCLUDE_CORE_SERIALIZABLE_H_
#define INCLUDE_CORE_SERIALIZABLE_H_

#include <cstddef>
#include <concepts>
#include <cstring>
#include<type_traits>

namespace FerryDB{
    namespace SerializableConcepts{
        template <typename T>
        concept Serializable = 
        std::is_fundamental<T>::value ||
        requires(T a, char* buffer){
            { a.size() } -> std::same_as<std::size_t>;
            { a.Serialize(buffer) } -> std::same_as<void>;
            { a.Deserialize(buffer) } -> std::same_as<void>;
        };
    }

    namespace Serializable{
        class Serializable{
        public:
            virtual std::size_t size() const = 0;
            virtual void Serialize(char*) = 0;
            virtual void Deserialize(char*) = 0;
        };
    };
}

#endif // INCLUDE_CORE_SERIALIZABLE_H_