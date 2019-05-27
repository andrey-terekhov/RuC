cmake_minimum_required(VERSION 3.0.0)

option(FLTK_IMPORTED "Import FLTK from git or use system-wide library" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(RUC_KEYWORDS_FILE ${PROJECT_SOURCE_DIR}/keywords.txt)
set(TEST_INPUTS_DIR ${PROJECT_SOURCE_DIR}/tests)
set(RUC_BINARY_DIR ${CMAKE_BINARY_DIR}/bin)

# add RuC
add_subdirectory(RuC)

# add RuC Virtual Machine
add_subdirectory(RuCVM)

# add utils
add_subdirectory(util)

if(FLTK_IMPORTED)
add_subdirectory(thirdparty/fltk)
endif()

# add UI
add_subdirectory(UI)
