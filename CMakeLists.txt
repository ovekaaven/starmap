cmake_minimum_required(VERSION 3.21)
project(starmap)

set(CMAKE_CXX_STANDARD 14)

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

find_package(Boost REQUIRED COMPONENTS iostreams)

add_executable(starmap starmap.cpp readbase.cpp readbase.h maths.h readbright.cpp readbright.h import.cpp import.h maths.cpp colors.cpp colors.h readgliese.cpp readgliese.h starlist.cpp starlist.h)
target_link_libraries(starmap ${wxWidgets_LIBRARIES} ${Boost_LIBRARIES})
