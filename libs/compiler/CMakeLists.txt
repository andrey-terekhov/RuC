cmake_minimum_required(VERSION 3.7.2)

project(compiler)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} preprocessor utils)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_link_libraries(${PROJECT_NAME} m)
endif()
