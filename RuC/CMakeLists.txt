cmake_minimum_required(VERSION 3.0.0)

project(ruc)

set(CMAKE_C_STANDARD 11)

file(GLOB SOURCE_FILES "*.c")
add_executable(ruc ${SOURCE_FILES})

if(MSVC)
    target_compile_options(ruc PRIVATE /W4 /WX)
else()
    target_compile_options(ruc PRIVATE -Wall -Wextra)
endif()

# Put the keywords.txt file near the driver
configure_file(${RUC_KEYWORDS_FILE} ${RUC_BINARY_DIR}/keywords.txt COPYONLY)

target_link_libraries(ruc ruc-util m)

set_target_properties(ruc
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
)

