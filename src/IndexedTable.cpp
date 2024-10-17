#include <iostream>
#include "IndexedTable.h"
#include <stdexcept>
#include <fstream>

// Default constructor
IndexedTable::IndexedTable()
    : TableName(""), IndexColumn(""), IndexColumnType(DataType::STRING) {}

bool IndexedTable::isValidIndexType(const std::string& value) const 
{
    try{
        switch(IndexColumnType){
            case DataType::INT:
                std::stoi(value);
                break;
            case DataType::FLOAT:
                std::stoi(value);
                break;
            case DataType::STRING:
                break;
        }
    }
    catch(...){
        return false;
    }
    return true;
}

IndexedTable::IndexedTable(const std::string& name, const std::string& indexCol, DataType indexType)
        : TableName(name), IndexColumn(indexCol), IndexColumnType(indexType) {}

void IndexedTable::AddRow(const std::string& indexValue, const TableRow& row){
    if(!isValidIndexType(indexValue)){
        throw std::invalid_argument("Invalid Index Value Type");
    }
    if(Rows.find(indexValue) != Rows.end()){
        throw std::invalid_argument("Index Value must be Unqiue");
    }
    Rows[indexValue] = row;
}

TableRow IndexedTable::GetRow(const std::string& indexValue) const {
    auto it = Rows.find(indexValue);
    if(it == Rows.end()){
        throw std::out_of_range("Row with given index not found.");
    }
    return it->second;
}

bool IndexedTable::rowExists(const std::string& indexValue) const {
    return Rows.find(indexValue) != Rows.end();
}

void NameSpace::AddIndexedTable(const std::string& tableName, const std::string& indexCol, DataType indexColType){
    if(Tables.find(tableName) != Tables.end()) {
        throw std::invalid_argument("Table with the given name already exists.");
    }
    Tables[tableName] = IndexedTable(tableName, indexCol, indexColType);
}

IndexedTable& NameSpace::getTable(const std::string& tableName){
    auto it = Tables.find(tableName);
    if(it == Tables.end()){
        throw std::out_of_range("Table with give name not found.");
    }
    return it->second;
}

void IndexedTable::AddColumn(const std::string& columnName, DataType columnType) {
    // Check if the column already exists
    if (Columns.find(columnName) != Columns.end()) {
        throw std::invalid_argument("Column already exists in the table");
    }

    Columns[columnName] = columnType;
    
    for (auto& [index, row] : Rows) {
        row.data[columnName] = ""; 
    }
}

// Serialize the table to a file
void IndexedTable::serialize(std::ofstream& outFile) const {
    // Write the table name, index column, and index column type
    size_t tableNameSize = TableName.size();
    outFile.write(reinterpret_cast<const char*>(&tableNameSize), sizeof(tableNameSize));
    outFile.write(TableName.c_str(), tableNameSize);

    size_t indexColSize = IndexColumn.size();
    outFile.write(reinterpret_cast<const char*>(&indexColSize), sizeof(indexColSize));
    outFile.write(IndexColumn.c_str(), indexColSize);

    outFile.write(reinterpret_cast<const char*>(&IndexColumnType), sizeof(IndexColumnType));

    // Write the number of rows
    size_t numRows = Rows.size();
    outFile.write(reinterpret_cast<const char*>(&numRows), sizeof(numRows));

    // Write each row
    for (const auto& [key, row] : Rows) {
        size_t keySize = key.size();
        outFile.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
        outFile.write(key.c_str(), keySize);

        // Write each row's data (each value for the row)
        size_t numColumns = row.data.size();
        outFile.write(reinterpret_cast<const char*>(&numColumns), sizeof(numColumns));
        for (const auto& [col, val] : row.data) {
            size_t colSize = col.size();
            size_t valSize = val.size();

            outFile.write(reinterpret_cast<const char*>(&colSize), sizeof(colSize));
            outFile.write(col.c_str(), colSize);
            outFile.write(reinterpret_cast<const char*>(&valSize), sizeof(valSize));
            outFile.write(val.c_str(), valSize);
        }
    }
}

// Deserialize the table from a file
void IndexedTable::deserialize(std::ifstream& inFile) {
    // Read the table name, index column, and index column type
    size_t tableNameSize;
    inFile.read(reinterpret_cast<char*>(&tableNameSize), sizeof(tableNameSize));
    TableName.resize(tableNameSize);
    inFile.read(&TableName[0], tableNameSize);

    size_t indexColSize;
    inFile.read(reinterpret_cast<char*>(&indexColSize), sizeof(indexColSize));
    IndexColumn.resize(indexColSize);
    inFile.read(&IndexColumn[0], indexColSize);

    inFile.read(reinterpret_cast<char*>(&IndexColumnType), sizeof(IndexColumnType));

    // Read the number of rows
    size_t numRows;
    inFile.read(reinterpret_cast<char*>(&numRows), sizeof(numRows));

    // Read each row
    for (size_t i = 0; i < numRows; ++i) {
        size_t keySize;
        inFile.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));

        std::string key(keySize, ' ');
        inFile.read(&key[0], keySize);

        size_t numColumns;
        inFile.read(reinterpret_cast<char*>(&numColumns), sizeof(numColumns));

        TableRow row;
        for (size_t j = 0; j < numColumns; ++j) {
            size_t colSize, valSize;

            // Read the column name
            inFile.read(reinterpret_cast<char*>(&colSize), sizeof(colSize));
            std::string col(colSize, ' ');
            inFile.read(&col[0], colSize);

            // Read the value
            inFile.read(reinterpret_cast<char*>(&valSize), sizeof(valSize));
            std::string val(valSize, ' ');
            inFile.read(&val[0], valSize);

            row.data[col] = val;
        }

        Rows[key] = row;
    }
}

// Save the table to a file
void IndexedTable::saveToFile(const std::string& fileName) const {
    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open()) {
        throw std::runtime_error("Unable to open file for writing");
    }
    serialize(outFile);
    outFile.close();
}

// Load a table from a file
IndexedTable IndexedTable::loadFromFile(const std::string& fileName) {
    IndexedTable table;
    std::ifstream inFile(fileName, std::ios::binary);
    if (!inFile.is_open()) {
        throw std::runtime_error("Unable to open file for reading");
    }
    table.deserialize(inFile);
    inFile.close();
    return table;
}


void NameSpace::AddColumn(const std::string& tableName, const std::string& columnName, DataType columnType) {
    auto it = Tables.find(tableName);
    if (it == Tables.end()) {
        throw std::out_of_range("Table with the given name does not exist.");
    }

    it->second.AddColumn(columnName, columnType);
}
