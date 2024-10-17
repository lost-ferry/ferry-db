#include <iostream>
#include <cassert>
#include "IndexedTable.h"

int main() {
    try {
        // Step 1: Create a Namespace
        NameSpace ns;

        // Step 2: Create a table with an index column (EmployeeTable, indexed by EmployeeID of type INT)
        ns.AddIndexedTable("EmployeeTable", "EmployeeID", DataType::INT);

        // Step 3: Add new columns to the table
        ns.AddColumn("EmployeeTable", "Name", DataType::STRING);
        ns.AddColumn("EmployeeTable", "Age", DataType::INT);

        // Try adding the same column again 
        try {
            ns.AddColumn("EmployeeTable", "Name", DataType::STRING); // Duplicate column
            assert(false); // If no exception, fail the test
        } catch (const std::invalid_argument& e) {
            std::cout << "Caught expected exception for duplicate column: " << e.what() << std::endl;
        }

        // Step 4: Add rows with valid index and column data
        TableRow row1;
        row1["Name"] = "Alice";
        row1["Age"] = "30";
        ns.getTable("EmployeeTable").AddRow("1", row1); // Valid row

        TableRow row2;
        row2["Name"] = "Bob";
        row2["Age"] = "25";
        ns.getTable("EmployeeTable").AddRow("2", row2); // Valid row

        // Try adding a row with a duplicate index value
        try {
            ns.getTable("EmployeeTable").AddRow("1", row2); // Duplicate index
            assert(false); // If no exception, fail the test
        } catch (const std::invalid_argument& e) {
            std::cout << "Caught expected exception for duplicate index: " << e.what() << std::endl;
        }

        // Step 5: Retrieve and Verify row data
        TableRow result1 = ns.getTable("EmployeeTable").GetRow("1");
        assert(result1["Name"] == "Alice");
        assert(result1["Age"] == "30");

        TableRow result2 = ns.getTable("EmployeeTable").GetRow("2");
        assert(result2["Name"] == "Bob");
        assert(result2["Age"] == "25");

        std::cout << "Test passed: Employee data retrieved and verified successfully." << std::endl;

        // Step 6: Try adding a row with an invalid index type 
        try {
            TableRow row3;
            row3["Name"] = "Charlie";
            row3["Age"] = "22";
            ns.getTable("EmployeeTable").AddRow("ABC", row3); // Invalid index 
            assert(false); // If no exception, fail the test
        } catch (const std::invalid_argument& e) {
            std::cout << "Caught expected exception for invalid index type: " << e.what() << std::endl;
        }

        // Test case for successfully adding rows 
        std::cout << "All test cases passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test case failed with exception: " << e.what() << std::endl;
    }

    return 0;
}