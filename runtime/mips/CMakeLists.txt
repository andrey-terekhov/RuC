cmake_minimum_required(VERSION 3.13.5)

project(runtime-mips)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "*.s")
file(COPY ${SRC} DESTINATION "${CMAKE_BINARY_DIR}/runtime-mips")

install(
    FILES ${SRC}
    DESTINATION ruc/runtime-mips
)

