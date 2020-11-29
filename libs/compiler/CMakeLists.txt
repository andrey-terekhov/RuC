cmake_minimum_required(VERSION 3.13.5)

project(compiler)


file(GLOB SRC "*.c")
file(GLOB HDR "*.h")

source_group("\\" FILES ${SRC} ${HDR})
add_library(${PROJECT_NAME} SHARED ${SRC} ${HDR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(${PROJECT_NAME} preprocessor utils)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_link_libraries(${PROJECT_NAME} m)
endif()
