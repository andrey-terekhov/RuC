# Analysis Server Protocol library
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

if (DEFINED ENABLE_ASP_LOCAL)
  add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/asp)
else ()
  FetchContent_Declare(
    asp
    GIT_TAG "simple_api"
    GIT_REPOSITORY "https://github.com/maximmenshikov/asp.git"
  )

  FetchContent_MakeAvailable(asp)
endif ()