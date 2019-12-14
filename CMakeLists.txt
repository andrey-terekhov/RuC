cmake_minimum_required(VERSION 3.0.0)

project(ruc)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


file(GLOB SRC "src/*.c")
add_executable(${PROJECT_NAME} ${SRC})


if(MSVC)
	file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src/keywords.txt" DESTINATION  "${CMAKE_BINARY_DIR}/Debug")
	file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/src/keywords.txt" DESTINATION  "${CMAKE_BINARY_DIR}/Release")
else()
	file(COPY "${CMAKE_SOURCE_DIR}/src/keywords.txt" DESTINATION  ${CMAKE_BINARY_DIR})
endif()


add_subdirectory("libs/compiler")

target_include_directories(${PROJECT_NAME} PRIVATE "libs/compiler")
target_link_libraries(${PROJECT_NAME} compiler)
