cmake_minimum_required(VERSION 3.0.0)

project(ruc)


option(ANALYSIS_ENABLED "Enable static analysis of input files" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TEST_INPUTS_DIR ${PROJECT_SOURCE_DIR}/tests)

# Put all libraries to one folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(-Werror=return-type -g)
endif()

if(ANALYSIS_ENABLED)
	include(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/aspsimple.cmake)
endif()


# Add libraries
add_subdirectory(libs)

# Add frontend
add_subdirectory(src)


enable_testing()
set(TEST_TIMEOUT 1)
file(GLOB TEST_INPUTS "${TEST_INPUTS_DIR}/*.c"
						"${TEST_INPUTS_DIR}/*/*.c"
						"${TEST_INPUTS_DIR}/*/*/*.c"
						"${TEST_INPUTS_DIR}/*/*/*/*.c")

foreach(TEST_INPUT ${TEST_INPUTS})
	add_test(NAME ${TEST_INPUT}
			 COMMAND "./ruc" ${TEST_INPUT}
			 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_tests_properties(${TEST_INPUT} PROPERTIES TIMEOUT ${TEST_TIMEOUT})
endforeach(TEST_INPUT)
