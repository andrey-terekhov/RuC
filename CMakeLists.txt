cmake_minimum_required(VERSION 3.13.5)

project(ruc)


# Put all libraries to one folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set RPATH for searching dynamic libraries in the same folder
set(CMAKE_INSTALL_RPATH $ORIGIN @loader_path)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


if(MSVC)
	# Starting in Visual Studio 2019 version 16.8, you may specify /std:c11
	add_compile_options(/W4 /WX /wd4204 /wd4221 /wd5105 /utf-8
						"$<$<VERSION_GREATER_EQUAL:MSVC_VERSION,1928>:/std:c11>"
						"$<$<NOT:$<CONFIG:Debug>>:/O2>"
						"$<$<CONFIG:Debug>:/Od>")

	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
	# Add POSIX flag to declare ‘fileno’ function
	# Enable -s flag for Release build to remove code info from exec
	add_compile_options(-Werror=return-type -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-const-variable -std=c11
						"$<$<NOT:$<CXX_COMPILER_ID:AppleClang,Clang>>:-posix>"
						"$<$<NOT:$<OR:$<CXX_COMPILER_ID:AppleClang,Clang>,$<CONFIG:Debug>>>:-s>"
						"$<$<NOT:$<CONFIG:Debug>>:-O2>"
						"$<$<CONFIG:Debug>:-g>")

	# Set _POSIX_C_SOURCE to declare ‘readlink’ function
	add_compile_definitions("$<$<NOT:$<CXX_COMPILER_ID:AppleClang,Clang>>:_POSIX_C_SOURCE=200112L>")
endif()

# Disable asserts for Release build
add_compile_definitions("$<$<NOT:$<CONFIG:Debug>>:NDEBUG>")


# Import testing exit code
if(DEFINED TESTING_EXIT_CODE)
	add_compile_definitions(TESTING_EXIT_CODE=${TESTING_EXIT_CODE})
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
