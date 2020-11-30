cmake_minimum_required(VERSION 3.13.5)

project(ruc)


# Put all libraries to one folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	#add_compile_options(/W4 /WX /utf-8 /wd5105)
	add_compile_options(/W4 /utf-8 /wd5105)

	# Starting in Visual Studio 2019 version 16.8, you may specify /std:c11
	if(MSVC_VERSION GREATER_EQUAL 1928)
		add_compile_options(/std:c11)
	endif()

	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(-Werror=return-type -Wall -Wextra -g -std=c11)
	
	if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
		add_compile_options(-posix)
		add_definitions(-D_POSIX_C_SOURCE=200112L)
	endif()
endif()


# Add libraries
add_subdirectory(libs)

# Add frontend
add_subdirectory(src)


function(get_all_targets _targets _dir)
	get_property(_subdirs DIRECTORY ${_dir} PROPERTY SUBDIRECTORIES)

	foreach(_subdir IN LISTS _subdirs)
		get_all_targets(${_targets} ${_subdir})
	endforeach()

	get_property(_sub_targets DIRECTORY ${_dir} PROPERTY BUILDSYSTEM_TARGETS)
	set(${_targets} ${${_targets}} ${_sub_targets} PARENT_SCOPE)
endfunction()


# Set install parameters
get_all_targets(targets .)
install(TARGETS ${targets}
		RUNTIME DESTINATION ${PROJECT_NAME}
		LIBRARY DESTINATION ${PROJECT_NAME})
