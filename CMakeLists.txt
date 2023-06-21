cmake_minimum_required(VERSION 3.23)
project(socc)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-stdlib=libc++ -fexperimental-library)
add_link_options(-stdlib=libc++ -fexperimental-library)

include(cmake/CPM.cmake)
CPMAddPackage("gh:TheLartians/Ccache.cmake@1.2.4")

if(NOT TARGET libsoasm)
    add_subdirectory(extern/SOASM)
endif()

add_library(libsocc src/var.cpp src/statement.cpp src/library.cpp include/socc/library.hpp)
target_link_libraries(libsocc libsoasm)
target_include_directories(libsocc PUBLIC include)

add_library(libsocc-soisv1 src/instrs.cpp)
target_link_libraries(libsocc-soisv1 libsocc soisv1)

add_executable(socc main.cpp)
target_link_libraries(socc libsocc-soisv1)