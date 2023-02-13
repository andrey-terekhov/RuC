cmake_minimum_required(VERSION 3.13.5)

project(runtime-mips)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "*.min.s")

FILE(COPY ${SRC} DESTINATION "${CMAKE_BINARY_DIR}/runtime-mips")
