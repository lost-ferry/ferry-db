cmake_minimum_required(VERSION 3.22)  # Ensure CMake version supports C++23

# Set the project name and version
project(ferrydb VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check the build type and set flags accordingly
# Use as: -DCMAKE_BUILD_TYPE=Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3 -Wall -fpic -ffinite-math-only -ftree-vectorize -march=native")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3 -Wall -fpic -ffinite-math-only -ftree-vectorize -march=native -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
else()
    message(WARNING "Unsupported build type: ${CMAKE_BUILD_TYPE}. Using default flags.")
endif()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

Include(FetchContent)

# Specify the source files for the library
set(SOURCES
    src/main.cpp
)

# Create the static library
add_library(ferrydb STATIC ${SOURCES})

# Specify the include directories for the library
target_include_directories(ferrydb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Optionally, set the version and SOVERSION
set_target_properties(ferrydb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Installation rules (optional)
install(TARGETS ferrydb
    EXPORT ferrydbExport
    ARCHIVE DESTINATION lib
)

install(EXPORT ferrydbExport
    FILE ferrydbConfig.cmake
    NAMESPACE ferrydb::
    DESTINATION lib/cmake/ferrydb
)

install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
