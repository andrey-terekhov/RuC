cmake_minimum_required(VERSION 3.7.2)

project(ruc)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put all libraries to one folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(-Werror=return-type -g)
endif()


option(ANALYSIS_ENABLED "Enable static analysis of input files" OFF)

if(ANALYSIS_ENABLED)
	include(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/aspsimple.cmake)
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
		LIBRARY DESTINATION ${PROJECT_NAME}
		ARCHIVE DESTINATION ${PROJECT_NAME})
