# FLTK library
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  fltk
  GIT_REPOSITORY "https://github.com/fltk/fltk"
)

FetchContent_MakeAvailable(
  fltk)
