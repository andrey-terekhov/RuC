cmake_minimum_required(VERSION 3.4)
project(RuC)
set(MAIN_EXE RuC.exe)
set(CMAKE_C_STANDARD 99)



set(CMAKE_REQUIRED_QUIET Off)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -ansi")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wno-error=unused-variable")

#Enable sanitizers support
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
set(SANITIZE_UNDEFINED On)
set(SANITIZE_ADDRESS On)
find_package(Sanitizers)

set(SOURCE_FILES
    RuC/global_vars.h
    RuC/Defs.h
    RuC/codegen.c
    RuC/codes.c
    RuC/error.c
    RuC/extdecl.c
    RuC/import.c
    RuC/main.c
    RuC/scaner.c
)


add_executable(${MAIN_EXE} ${SOURCE_FILES})
target_link_libraries(${MAIN_EXE} m)
add_sanitizers(${MAIN_EXE})