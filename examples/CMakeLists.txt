cmake_minimum_required(VERSION 3.0.0)
project(arena_mr_examples VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /W4")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DEBUG /Od") # Additional flags for C++

include(FetchContent)
FetchContent_Declare(arena_mr SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_MakeAvailable(arena_mr)


file(GLOB EXAMPLES "*.cpp" )

foreach(example ${EXAMPLES})
    get_filename_component(executable_name ${example} NAME_WE)
    add_executable(${executable_name} ${example})
    target_link_libraries(${executable_name} PRIVATE arena_mr)
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
