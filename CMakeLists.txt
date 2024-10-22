cmake_minimum_required(VERSION 3.22)
project(ferrydb VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set policy for Boost ROOT variables
cmake_policy(SET CMP0144 NEW)

# Enable testing with CTest
include(CTest)
enable_testing()

# Check the build type and set flags accordingly
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

include_directories("src/Private")

# Create ferrydb interface library
add_library(ferrydb INTERFACE)
target_include_directories(ferrydb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Private>
    $<INSTALL_INTERFACE:src/Private>
)

set_target_properties(ferrydb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

set(Boost_USE_DEBUG_LIBS         ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF) 
find_package(Boost)

if(Boost_FOUND)
    target_link_libraries(ferrydb INTERFACE interprocess)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Could not find Boost.")
endif()

add_executable(SimpleGraphTest "tests/simple_graph_test.cpp")
add_executable(ManagerLoadSaveTest "tests/manager_load_save_test.cpp")

add_test(NAME simple_graph_test COMMAND SimpleGraphTest)
add_test(NAME manager_load_save_test COMMAND ManagerLoadSaveTest)

add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS SimpleGraphTest ManagerLoadSaveTest)

add_dependencies(run_unit_tests SimpleGraphTest ManagerLoadSaveTest)
