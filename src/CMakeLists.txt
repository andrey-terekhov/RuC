cmake_minimum_required(VERSION 3.13.5)

project(ruc)


file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "*.c")
file(GLOB_RECURSE HDR CONFIGURE_DEPENDS "*.h")

source_group("\\" FILES ${SRC} ${HDR})
add_executable(${PROJECT_NAME} ${SRC} ${HDR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(${PROJECT_NAME} compiler utils)
if (ENABLE_ANALYSIS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE -DENABLE_ANALYSIS)
	target_link_libraries(${PROJECT_NAME} analysis)
endif ()
