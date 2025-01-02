#ifndef INDEXED_TABLE_H
#define INDEXED_TABLE_H

#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <optional>
#include <type_traits>
#include <cstring>
#include <stdexcept>

namespace FerryDB {

    template <typename T>
    struct is_printable {
        template <typename U>
        static auto test(int) -> decltype(std::cout << std::declval<U>(), std::true_type());

        template <typename>
        static auto test(...) -> std::false_type;

        static constexpr bool value = decltype(test<T>(0))::value;
    };

    // Indexed Table Class
    template <typename IndexType, typename... ColumnTypes>
    class IndexedTable {
        static_assert(is_printable<IndexType>::value, "IndexType must be printable.");
        static_assert((is_printable<ColumnTypes>::value && ...), "All ColumnTypes must be printable.");

    public:
        using RowType = std::tuple<IndexType, ColumnTypes...>;

    private:
        std::unordered_map<IndexType, RowType> table;    
        std::vector<IndexType> index_order;               
        std::vector<std::string> column_names;         

    public:
        IndexedTable() = default;

        void InsertRow(IndexType index, ColumnTypes... values);

        void UpdateRow(IndexType index, ColumnTypes... values);

        void DeleteRow(IndexType index);

        void DropTable();

        template <typename NewColumnType>
        void AddColumn(const std::string& column_name);

        void RemoveColumn(const std::string& column_name);

        RowType GetRow(IndexType index) const;

        size_t Serialize(char* buffer) const;

        void Deserialize(const char* buffer, size_t buffer_length);

        void Print() const;

        size_t Size() const {
            return table.size();
        }

        size_t SerializerSize() const {
            size_t total_size = sizeof(size_t);
            total_size += table.size() * (sizeof(IndexType) + sizeof(size_t)); 
            for (const auto& index : index_order) {
                const auto& row = table.at(index);
                std::apply([&](const auto&... args) {
                    ((total_size += sizeof(args)), ...);
                }, row);
            }
            return total_size;
        }

    private:
        void PrintRow(const RowType& row) const;
    };
} 

#endif