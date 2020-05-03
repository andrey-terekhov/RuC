cmake_minimum_required(VERSION 3.0.0)

project(compiler)


file(GLOB SRC "*.c" "preprocessor/*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/preprocessor
	${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} utils)


if(NOT MSVC)
	target_link_libraries(${PROJECT_NAME} m)
endif()
