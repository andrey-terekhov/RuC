# This library wraps all RuC libraries, ensuring that their functionality is compilable
# in C++ context
cmake_minimum_required(VERSION 3.13.5)

project(ruccpp)

add_library(${PROJECT_NAME} SHARED wrap.cpp)

target_link_libraries(${PROJECT_NAME}
					  PUBLIC
					  	compiler
					  	utils)
if (RUC_CPPWRAP_CHECK_PREPROCESSOR)
	target_link_libraries(${PROJECT_NAME}
						  PUBLIC
						  	preprocessor)
endif ()