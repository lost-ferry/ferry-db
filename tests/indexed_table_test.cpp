#include <iostream>
#include <cassert>
#include <string>
#include "IndexedTableDescription.h"

void test_indexed_table_serialization() {
    using Table = FerryDB::IndexedTable<int, std::string, double>;
    Table table;

    std::cout << "Inserting rows..." << std::endl;
    table.InsertRow(1, "Row1", 1.1);
    table.InsertRow(2, "Row2", 2.2);
    table.InsertRow(3, "Row3", 3.3);

    size_t original_size = table.Size();

    size_t buffer_size = table.SerializerSize();

    char* buffer = new char[buffer_size];

    try {
        std::cout << "Starting serialization..." << std::endl;
        size_t serialized_size = table.Serialize(buffer);

        table.DropTable();

        std::cout << "Starting deserialization..." << std::endl;
        table.Deserialize(buffer, serialized_size);

        size_t new_size = table.Size();
        assert(new_size == original_size);
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception occurred." << std::endl;
    }

    delete[] buffer;
    std::cout << "Done" << std::endl;
}

int main() {
    test_indexed_table_serialization();
    return 0;
}
