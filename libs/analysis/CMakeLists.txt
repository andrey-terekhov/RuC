cmake_minimum_required(VERSION 3.13.5)

project(analysis)

add_library(${PROJECT_NAME}
			STATIC
				analysis.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} utils asp)
