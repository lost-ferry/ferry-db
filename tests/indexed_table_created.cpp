#include "IndexedTable.h"
#include <iostream>

int main() {
    // Create a namespace
    NameSpace myNamespace;

    // Add a table with an integer index column
    myNamespace.AddIndexedTable("Users", "UserID", DataType::INT);

    // Get the table
    IndexedTable& userTable = myNamespace.getTable("Users");

    // Create a row and add data
    TableRow row1;
    row1["Name"] = "Alice";
    row1["Age"] = "30";

    // Add the row with a unique index value
    userTable.AddRow("1", row1);

    // Create another row
    TableRow row2;
    row2["Name"] = "Bob";
    row2["Age"] = "25";

    // Add the second row with a different unique index value
    userTable.AddRow("2", row2);

    // Retrieve a row
    TableRow retrievedRow = userTable.GetRow("1");
    std::cout << "Retrieved Row: " << retrievedRow["Name"] << ", Age: " << retrievedRow["Age"] << std::endl;

    return 0;
}