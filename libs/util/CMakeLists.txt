cmake_minimum_required(VERSION 3.0.0)

project(util)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} STATIC ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if(MSVC)
  	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
