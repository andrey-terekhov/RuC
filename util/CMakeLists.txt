cmake_minimum_required(VERSION 3.0.0)

project(ruc-util)

set(CMAKE_C_STANDARD 11)

file(GLOB SOURCE_FILES "*.c")
add_library(ruc-util ${SOURCE_FILES})

if(MSVC)
  	target_compile_options(ruc-util PRIVATE /W4 /WX)
else()
  	target_compile_options(ruc-util PRIVATE -Wall -Wextra)
endif()

target_include_directories(ruc-util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
