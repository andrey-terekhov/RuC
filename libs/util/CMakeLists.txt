cmake_minimum_required(VERSION 3.0.0)

project(compiler-util)

set(CMAKE_C_STANDARD 11)

file(GLOB SOURCE_FILES "*.c")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

if(MSVC)
  	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
