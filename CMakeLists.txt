cmake_minimum_required(VERSION 3.13.5)

project(ruc)


# Put all libraries to one folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/utf-8)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(-Wno-unused-result)
endif()


# Add sources
add_subdirectory(src)


# Set Visual Studio startup project
set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
