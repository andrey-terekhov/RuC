cmake_minimum_required(VERSION 3.0.0)

project(ruc)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


file(GLOB SRC "*.c")

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} compiler compiler-util)

if(ANALYSIS_ENABLED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ANALYSIS_ENABLED)
    target_link_libraries(${PROJECT_NAME} aspsimple)
endif()
