#ifndef INDEXED_TABLE_DESCRIPTION_H
#define INDEXED_TABLE_DESCRIPTION_H

#include "IndexedTable.h"
#include "IndexedTableSerializable.h"

namespace FerryDB {

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::InsertRow(IndexType index, ColumnTypes... values) {
        if (table.find(index) != table.end()) {
            throw std::runtime_error("Index already exists.");
        }
        table[index] = std::make_tuple(index, values...);
        index_order.push_back(index);
    }

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::UpdateRow(IndexType index, ColumnTypes... values) {
        auto it = table.find(index);
        if (it == table.end()) {
            throw std::runtime_error("Index not found.");
        }
        table[index] = std::make_tuple(index, values...);
    }

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::DeleteRow(IndexType index) {
        table.erase(index);
        index_order.erase(std::remove(index_order.begin(), index_order.end(), index), index_order.end());
    }

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::DropTable() {
        table.clear();
        index_order.clear();
    }

    template <typename IndexType, typename... ColumnTypes>
    template <typename NewColumnType>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::AddColumn(const std::string& column_name) {
        column_names.push_back(column_name);
    }

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::RemoveColumn(const std::string& column_name) {
        auto it = std::remove(column_names.begin(), column_names.end(), column_name);
        if (it != column_names.end()) {
            column_names.erase(it, column_names.end());
        }
    }

    template <typename IndexType, typename... ColumnTypes>
    typename FerryDB::IndexedTable<IndexType, ColumnTypes...>::RowType 
    FerryDB::IndexedTable<IndexType, ColumnTypes...>::GetRow(IndexType index) const {
        auto it = table.find(index);
        if (it != table.end()) {
            return it->second; 
        }
        throw std::runtime_error("Index not found."); 
    }

    template <typename IndexType, typename... ColumnTypes>
    size_t FerryDB::IndexedTable<IndexType, ColumnTypes...>::Serialize(char* buffer) const {
        size_t offset = 0;
        size_t row_count = table.size();
        
        // Write the number of rows to the buffer
        std::memcpy(buffer + offset, &row_count, sizeof(row_count));
        offset += sizeof(row_count);

        for (const auto& index : index_order) {
            std::memcpy(buffer + offset, &index, sizeof(IndexType));
            offset += sizeof(IndexType);
            
            const auto& row = table.at(index);
            // Calculate the name length
            std::string name = std::get<1>(row); // Assuming the second element is the name
            size_t name_length = name.size();
            
            // Write the name length to the buffer
            std::memcpy(buffer + offset, &name_length, sizeof(name_length));
            offset += sizeof(name_length);
            
            // Write the name data to the buffer
            std::memcpy(buffer + offset, name.data(), name_length);
            offset += name_length;

            // Write the value to the buffer
            double value = std::get<2>(row); // Assuming the third element is the value
            std::memcpy(buffer + offset, &value, sizeof(double));
            offset += sizeof(double);
        }
        return offset;
    }


    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::Deserialize(const char* buffer, size_t buffer_length) {
        size_t offset = 0;
        size_t row_count;
        std::memcpy(&row_count, buffer + offset, sizeof(row_count));
        offset += sizeof(row_count);

        for (size_t i = 0; i < row_count; ++i) {
            IndexType id;
            std::memcpy(&id, buffer + offset, sizeof(IndexType));
            offset += sizeof(IndexType);
            size_t name_length;
            std::memcpy(&name_length, buffer + offset, sizeof(name_length));
            offset += sizeof(name_length);
            if (offset + name_length > buffer_length) {
                throw std::runtime_error("Buffer is too small for the string data.");
            }
            std::string name(buffer + offset, name_length);
            offset += name_length;
            double value;
            std::memcpy(&value, buffer + offset, sizeof(double));
            offset += sizeof(double);
            InsertRow(id, name, value);
        }
    }


    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::Print() const {
        for (const auto& index : index_order) {
            const auto& row = table.at(index);
            PrintRow(row);
        }
    }

    template <typename IndexType, typename... ColumnTypes>
    void FerryDB::IndexedTable<IndexType, ColumnTypes...>::PrintRow(const RowType& row) const {
        std::apply([](auto&&... args) { ((std::cout << args << " "), ...); }, row);
    }

    // SerializableIndexedTable Implementation
    template <typename KeyType, typename ValueType>
    class SerializableIndexedTable : public IndexedTableSerializableConcepts::SerializableClass<SerializableIndexedTable<KeyType, ValueType>> {
    
    private:
        std::unordered_map<KeyType, ValueType> TableData;

    public:
        SerializableIndexedTable() = default;
        ~SerializableIndexedTable() = default;

        // Insert method
        void Insert(const KeyType& key, const ValueType& value) 
        requires IndexedTableSerializableConcepts::Serializable<KeyType> &&
                 IndexedTableSerializableConcepts::Serializable<ValueType> {
            TableData[key] = value;
        }

        // Get method
        ValueType Get(const KeyType& key) const 
        requires IndexedTableSerializableConcepts::Serializable<KeyType> {
            return TableData.at(key);
        }

        // SerializerSize method
        std::size_t SerializerSize() const 
        requires IndexedTableSerializableConcepts::Serializable<KeyType> &&
                 IndexedTableSerializableConcepts::Serializable<ValueType> {
            std::size_t totalSize = sizeof(IndexedTableSerializable::SerializedData);
            for (const auto& [key, value] : TableData) {
                totalSize += key.SerializerSize() + value.SerializerSize();
            }
            return totalSize;
        }

        // Serialize method
        static std::variant<IndexedTableSerializable::SerializedData, IndexedTableSerializable::SerializableError> Serialize(const SerializableIndexedTable<KeyType, ValueType>& tableToSerialize) 
        requires IndexedTableSerializableConcepts::Serializable<KeyType> &&
                 IndexedTableSerializableConcepts::Serializable<ValueType> {
            using SerializedData = IndexedTableSerializable::SerializedData;
            size_t size = tableToSerialize.SerializerSize();
            SerializedData serializedData(size);

            char* currentPtr = serializedData.GetDataPtr();
            for (const auto& [key, value] : tableToSerialize.TableData) {
                auto serializedKey = key.Serialize();
                if (std::holds_alternative<IndexedTableSerializable::SerializableError>(serializedKey)) {
                    return std::get<IndexedTableSerializable::SerializableError>(serializedKey);
                }
                std::memcpy(currentPtr, std::get<SerializedData>(serializedKey).GetDataPtr(), key.SerializerSize());
                currentPtr += key.SerializerSize();

                auto serializedValue = value.Serialize();
                if (std::holds_alternative<IndexedTableSerializable::SerializableError>(serializedValue)) {
                    return std::get<IndexedTableSerializable::SerializableError>(serializedValue);
                }
                std::memcpy(currentPtr, std::get<SerializedData>(serializedValue).GetDataPtr(), value.SerializerSize());
                currentPtr += value.SerializerSize();
            }
            return serializedData;
        }

        // Deserialize method
        static std::variant<SerializableIndexedTable<KeyType, ValueType>, IndexedTableSerializable::SerializableError> Deserialize(const IndexedTableSerializable::SerializedData& buffer) 
        requires IndexedTableSerializableConcepts::Serializable<KeyType> &&
                 IndexedTableSerializableConcepts::Serializable<ValueType> {
            SerializableIndexedTable<KeyType, ValueType> deserializedTable;
            const char* currentPtr = buffer.GetDataPtr();

            // Deserialize Key-Value pairs
            while (currentPtr < buffer.GetDataPtr() + buffer.GetLength()) {
                KeyType key;
                auto deserializedKey = key.Deserialize(IndexedTableSerializable::SerializedData(currentPtr, key.SerializerSize()));
                if (std::holds_alternative<IndexedTableSerializable::SerializableError>(deserializedKey)) {
                    return std::get<IndexedTableSerializable::SerializableError>(deserializedKey);
                }
                currentPtr += key.SerializerSize();

                ValueType value;
                auto deserializedValue = value.Deserialize(IndexedTableSerializable::SerializedData(currentPtr, value.SerializerSize()));
                if (std::holds_alternative<IndexedTableSerializable::SerializableError>(deserializedValue)) {
                    return std::get<IndexedTableSerializable::SerializableError>(deserializedValue);
                }
                currentPtr += value.SerializerSize();

                deserializedTable.Insert(key, value);
            }

            return deserializedTable;
        }
    };
}

#endif // INDEXED_TABLE_DESCRIPTION_H
