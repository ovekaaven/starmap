cmake_minimum_required(VERSION 3.21)
project(starmap)

set(CMAKE_CXX_STANDARD 14)

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(starmap starmap.cpp import.cpp)
target_link_libraries(starmap ${wxWidgets_LIBRARIES})
