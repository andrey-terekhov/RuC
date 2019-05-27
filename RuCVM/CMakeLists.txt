cmake_minimum_required(VERSION 3.0.0)

project(ruc-vm)

set(CMAKE_C_STANDARD 11)

file(GLOB SOURCE_FILES "*.c")
add_executable(ruc-vm ${SOURCE_FILES})

find_package (Threads REQUIRED)

if(MSVC)
  	target_compile_options(ruc-vm PRIVATE /W4 /WX)
else()
  	target_compile_options(ruc-vm PRIVATE -Wall -Wextra)
endif()

target_link_libraries(ruc-vm ruc-util m ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(ruc-vm
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
)
