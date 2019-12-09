cmake_minimum_required(VERSION 3.0.0)

project(ruc)

file(GLOB SRC "src/*.c")

add_executable(${PROJECT_NAME} ${SRC})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_SOURCE_DIR}/src/keywords.txt $<TARGET_FILE_DIR:${PROJECT_NAME}>)

add_subdirectory("libs/compiler")

target_include_directories(${PROJECT_NAME} PRIVATE "libs/compiler")
target_link_libraries(${PROJECT_NAME} compiler)
