# FLTK library
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  fltk
  GIT_REPOSITORY "https://github.com/fltk/fltk"
)

FetchContent_GetProperties(fltk)
if (NOT fltk_POPULATED)
    FetchContent_Populate(fltk)
    add_subdirectory(${fltk_SOURCE_DIR} ${fltk_BINARY_DIR})
endif()
