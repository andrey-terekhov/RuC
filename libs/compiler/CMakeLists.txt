cmake_minimum_required(VERSION 3.0.0)

project(compiler)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} util)


if(NOT MSVC)
	target_link_libraries(${PROJECT_NAME} m)
endif()
