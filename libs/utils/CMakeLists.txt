cmake_minimum_required(VERSION 3.7.2)

project(utils)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /utf-8)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wextra)
endif()
