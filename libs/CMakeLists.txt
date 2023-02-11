cmake_minimum_required(VERSION 3.13.5)


# Add utils	library
add_subdirectory(utils)

# Add preprocessor library
add_subdirectory(preprocessor)

# Add compiler library
add_subdirectory(compiler)

# Add C++ wrapper library
add_subdirectory(cpp-wrap)

if(ENABLE_ANALYSIS)
	# Add analysis library
	add_subdirectory(analysis)
endif()
