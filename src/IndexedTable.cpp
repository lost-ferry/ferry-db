#include <iostream>
#include "IndexedTable.h"

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

void IndexedTable::addRow(const std::string& indexValue, const TableRow& row){
    if(!isValidIndexType(indexValue)){
        throw std::invalid_argument("Invalid Index Value Type");
    }
    if(rows.find(indexValue) != rows.end()){
        throw std::invalid_argument("Index Value must be Unqiue");
    }
    rows[indexValue] = row;
}

TableRow IndexedTable::getRow(const std::string& indexValue) const {
    auto it = rows.find(indexValue);
    if(it == rows.end()){
        throw std::out_of_range("Row with given index not found.");
    }
    return it->second;
}

bool IndexedTable::rowExists(const std::string& indexValue) const {
    return rows.find(indexValue) != rows.end();
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

