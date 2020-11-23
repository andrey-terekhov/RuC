cmake_minimum_required(VERSION 3.0.0)

project(preprocessor)


file(GLOB SRC "*.c")
add_library(${PROJECT_NAME} SHARED ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} utils)


if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
else()
	target_link_libraries(${PROJECT_NAME} m)
endif()
