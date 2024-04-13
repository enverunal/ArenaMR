cmake_minimum_required(VERSION 3.0.0)
project(arena_mr VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
        INTERFACE ${PROJECT_SOURCE_DIR}/include)