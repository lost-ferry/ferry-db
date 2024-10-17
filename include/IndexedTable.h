#ifndef INDEXED_TABLE_H
#define INDEXED_TABLE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

enum class DataType {
    INT, 
    FLOAT,
    STRING
};

class TableRow {
    public:
        std::unordered_map<std::string, std::string> data;
        std::string& operator[](const std::string& key){
            return data[key];
        }
};

class IndexedTable {
    private:
        std::string TableName;
        std::string IndexColumn;
        DataType IndexColumnType;
        std::unordered_map<std::string, TableRow> Rows;
        std::unordered_map<std::string, DataType> Columns; 
        
        bool isValidIndexType(const std::string& value) const;
    public:
        IndexedTable(); // Default constructor

        IndexedTable(const std::string& name, const std::string& indexCol, DataType indexType);

        // Method to add a new column
        void AddColumn(const std::string& columnName, DataType columnType);

        //Method to add new row 
        void AddRow(const std::string& indexValue, const TableRow& row);

        //Method to retrieve a row by Index
        TableRow GetRow(const std::string& indexValue) const;

        bool rowExists(const std::string& indexValue) const;

        // Serialization methods
        void serialize(std::ofstream& outFile) const;
        void deserialize(std::ifstream& inFile);

        // Method to save table to a file
        void saveToFile(const std::string& fileName) const;

        // Method to load table from a file
        static IndexedTable loadFromFile(const std::string& fileName);
};

class NameSpace{
    private:
        std::unordered_map<std::string, IndexedTable> Tables;
    public:
        void AddIndexedTable(const std::string& tableName, const std::string& indexCol, DataType indexColType);
        IndexedTable& getTable(const std::string& tableName);
        
        void AddColumn(const std::string& tableName, const std::string& columnName, DataType columnType);
};

#endif 