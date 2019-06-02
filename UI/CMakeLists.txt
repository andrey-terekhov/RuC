cmake_minimum_required(VERSION 3.0.0)

project(ruc-ui)

file(GLOB SOURCE_FILES "*.cpp")
add_executable(ruc-ui ${SOURCE_FILES})

if(MSVC)
  	target_compile_options(ruc-ui PRIVATE /W4 /WX)
else()
  	target_compile_options(ruc-ui PRIVATE -Wall -Wextra)
endif()

set_target_properties(ruc-ui
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${RUC_BINARY_DIR}"
)

target_link_libraries(ruc-ui PRIVATE ruc-util)

if(FLTK_IMPORTED)
    get_target_property(FLTK_BINARY_DIR fltk BINARY_DIR)
    get_target_property(FLTK_SOURCE_DIR fltk SOURCE_DIR)
    message("fltk imported ${FLTK_BINARY_DIR} / ${FLTK_SOURCE_DIR}")
    target_include_directories(ruc-ui PRIVATE ${FLTK_BINARY_DIR}/.. ${FLTK_SOURCE_DIR}/..)
else()
    message("fltk system-wide")
    find_package (FLTK REQUIRED)
    target_include_directories(ruc-ui PRIVATE ${FLTK_INCLUDE_DIRS})
endif()

target_link_libraries(ruc-ui PRIVATE fltk)
if(APPLE)
    target_link_libraries(ruc-ui PRIVATE "-framework Foundation" "-framework Cocoa" objc)
endif()