cmake_minimum_required(VERSION 3.7.2)

project(ruc)


file(GLOB SRC "*.c")
add_executable(${PROJECT_NAME} ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} compiler utils)
