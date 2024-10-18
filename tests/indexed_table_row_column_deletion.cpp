#include <iostream>
#include <cassert>
#include "IndexedTable.h"

int main() {
    try {
        // Step 1: Create Namespace
        NameSpace ns;

        // Step 2: Create table with an index column 
        ns.AddIndexedTable("EmployeeTable", "EmployeeID", DataType::INT);
        std::cout << "Created table 'EmployeeTable' with index 'EmployeeID'.\n";

        // Step 3: Add new columns 
        ns.AddColumn("EmployeeTable", "Name", DataType::STRING);
        ns.AddColumn("EmployeeTable", "Age", DataType::INT);
        ns.AddColumn("EmployeeTable", "Department", DataType::STRING);
        std::cout << "Added columns 'Name', 'Age', and 'Department' to 'EmployeeTable'.\n";

        // Step 4: Add rows with valid index and column data
        TableRow row1;
        row1["Name"] = "Alice";
        row1["Age"] = "30";
        row1["Department"] = "HR";
        ns.getTable("EmployeeTable").AddRow("1", row1); // Valid row

        TableRow row2;
        row2["Name"] = "Bob";
        row2["Age"] = "25";
        row2["Department"] = "Engineering";
        ns.getTable("EmployeeTable").AddRow("2", row2); // Valid row

        // Print table after adding rows
        std::cout << "Table after adding rows:\n";

        // Step 5: Update a row with new data
        std::unordered_map<std::string, std::string> updatedRow;
        updatedRow["Age"] = "26";  
        ns.UpdateRow("EmployeeTable", "2", updatedRow);

        // Print table after updating a row
        std::cout << "Table after updating Bob's age:\n";

        // Step 6: Delete a row
        ns.DeleteRow("EmployeeTable", "1");
        std::cout << "Deleted row with index '1'.\n";

        // Print table after deleting a row
        std::cout << "Table after deleting Alice's row:\n";

        // Step 7: Remove a column
        ns.RemoveColumn("EmployeeTable", "Department");
        std::cout << "Removed column 'Department' from 'EmployeeTable'.\n";

        // Print table after removing a column
        std::cout << "Table after removing 'Department' column:\n";

        // Step 9: Drop the table entirely
        ns.DropIndexedTable("EmployeeTable");
        std::cout << "Dropped 'EmployeeTable'.\n";

        std::cout << "All test cases passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test case failed with exception: " << e.what() << std::endl;
    }

    return 0;
}
