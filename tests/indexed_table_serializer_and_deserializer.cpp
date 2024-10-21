#include <iostream>
#include <cassert>
#include <vector>
#include <fstream>
#include "IndexedTable.h"  // Make sure this path is correct

int main() {
    std::cout << "Starting Tests for IndexedTable Serialization \n";

    // Create an instance of IndexedTable
    IndexedTable table("EmployeeTable", "ID", DataType::INT);

    // Add some columns
    table.AddColumn("Name", DataType::STRING);
    table.AddColumn("Age", DataType::INT);
    table.AddColumn("Salary", DataType::FLOAT);

    // Add some rows
    TableRow row1;
    row1.data["Name"] = "Alice";
    row1.data["Age"] = "30";
    row1.data["Salary"] = "50000";
    table.AddRow("1", row1);

    TableRow row2;
    row2.data["Name"] = "Bob";
    row2.data["Age"] = "40";
    row2.data["Salary"] = "60000";
    table.AddRow("2", row2);

    // Check initial data before serialization
    std::cout << "Table before serialization \n";
    assert(table.rowExists("1"));
    assert(table.rowExists("2"));
    assert(table.GetRow("1").data["Name"] == "Alice");
    assert(table.GetRow("2").data["Name"] == "Bob");

    // Serialize the table to a buffer
    std::vector<char> buffer;
    std::ofstream outFile("test_table.dat", std::ios::binary);
    if (outFile.is_open()) {
        table.serialize(outFile);
        outFile.close();
    } else {
        std::cerr << "Error opening file for writing." << std::endl;
        return 1;
    }

    // Now, let's deserialize from the file
    IndexedTable loadedTable;
    std::ifstream inFile("test_table.dat", std::ios::binary);
    if (inFile.is_open()) {
        loadedTable.deserialize(inFile);
        inFile.close();
    } else {
        std::cerr << "Error opening file for reading." << std::endl;
        return 1;
    }

    // Check data after deserialization
    std::cout << "Table after de-serialization \n";
    assert(loadedTable.rowExists("1"));
    assert(loadedTable.rowExists("2"));
    assert(loadedTable.GetRow("1").data["Name"] == "Alice");
    assert(loadedTable.GetRow("2").data["Name"] == "Bob");

    std::cout << "All assertions passed. Test completed successfully!\n";

    return 0;
}
