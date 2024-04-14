cmake_minimum_required(VERSION 3.0.0)
project(arena_mr_examples VERSION 0.1.0)

if(MSVC)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W4")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG /Od") # Additional flags for C++
else()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -pedantic -Werror")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g") # Additional flags for C++
endif()

include(FetchContent)
FetchContent_Declare(arena_mr SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_MakeAvailable(arena_mr)


file(GLOB EXAMPLES "*.cpp" )
foreach(example ${EXAMPLES})
    get_filename_component(executable_name ${example} NAME_WE)
    add_executable(${executable_name} ${example})
    target_link_libraries(${executable_name} PRIVATE arena_mr)
endforeach()