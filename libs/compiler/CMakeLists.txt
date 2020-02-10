cmake_minimum_required(VERSION 3.0.0)

project(compiler)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ".")


if(NOT MSVC)
	target_link_libraries(${PROJECT_NAME} m)
endif()
