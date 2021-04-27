cmake_minimum_required(VERSION 3.13.5)

project(ruc)


file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "*.c")
file(GLOB_RECURSE HDR CONFIGURE_DEPENDS "*.h")

source_group("\\" FILES ${SRC} ${HDR})
add_executable(${PROJECT_NAME} ${SRC} ${HDR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy
					"${CMAKE_SOURCE_DIR}/keywords.txt"
					"${CMAKE_BINARY_DIR}/$<IF:$<CONFIG:Debug>,Debug,Release>")
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy
				"${CMAKE_SOURCE_DIR}/keywords.txt"
				"${CMAKE_BINARY_DIR}")

	target_link_libraries(${PROJECT_NAME} m)
endif()
