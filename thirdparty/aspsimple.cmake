# Simple Analysis Server Protocol library
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

set(USR_DIR_ASPSIMPLE ${CMAKE_CURRENT_BINARY_DIR}/usr/aspsimple)

FetchContent_Declare(
  aspsimple
  GIT_REPOSITORY "https://github.com/maximmenshikov/aspsimple.git"
)

FetchContent_MakeAvailable(
  aspsimple)
