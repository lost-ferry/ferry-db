#ifndef INCLUDE_CORE_SERIALIZABLE_H_
#define INCLUDE_CORE_SERIALIZABLE_H_

#include <concepts>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace FerryDB {
namespace SerializableConcepts {

template <typename T>
using is_string_like = std::disjunction<
    std::is_same<std::decay_t<T>, std::string>,
    std::is_same<std::decay_t<T>, std::__cxx11::basic_string<char>>>;

template <typename T>
concept Serializable = std::is_fundamental<T>::value ||
                       is_string_like<T>::value || requires(T a, char *buffer) {
                         { a.SerializerSize() } -> std::same_as<std::size_t>;
                         { a.Serialize(buffer) } -> std::same_as<void>;
                         { a.Deserialize(buffer) } -> std::same_as<void>;
                       };
} // namespace SerializableConcepts

namespace Serializable {
class Serializable {
public:
  virtual std::size_t SerializerSize() const = 0;
  virtual void Serialize(char *) = 0;
  virtual void Deserialize(char *) = 0;
};
}; // namespace Serializable
} // namespace FerryDB

#endif // INCLUDE_CORE_SERIALIZABLE_H_