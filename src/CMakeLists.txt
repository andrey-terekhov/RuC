cmake_minimum_required(VERSION 3.13.5)

project(main)


file(GLOB SRC "*.c")
file(GLOB HDR "*.h")

source_group("\\" FILES ${SRC} ${HDR})
add_executable(${PROJECT_NAME} ${SRC} ${HDR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(${PROJECT_NAME} compiler utils)
