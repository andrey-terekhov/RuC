cmake_minimum_required(VERSION 3.13.5)

project(utils)


file(GLOB SRC "*.c")
file(GLOB HDR "*.h")

source_group("\\" FILES ${SRC} ${HDR})
add_library(${PROJECT_NAME} SHARED ${SRC} ${HDR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
