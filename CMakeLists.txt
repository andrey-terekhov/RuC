cmake_minimum_required(VERSION 3.0.0)

project(RuC)

set(CMAKE_C_STANDARD 11)

if(ENABLE_SANITIZERS)
  set(ENABLE_SANITIZERS "-g -fsanitize=address,undefined -fno-omit-frame-pointer")
endif(ENABLE_SANITIZERS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra ${ENABLE_SANITIZERS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${ENABLE_SANITIZERS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${ENABLE_SANITIZERS}")

set(EXECUTABLE_NAME ruc-compiler)

set(SOURCES_DIR     ${PROJECT_SOURCE_DIR}/RuC)
set(HEADERS_DIR     ${PROJECT_SOURCE_DIR}/RuC)
set(TEST_INPUTS_DIR ${PROJECT_SOURCE_DIR}/tests)

set(SOURCES ${SOURCES_DIR}/scaner.c
            ${SOURCES_DIR}/preprocess.c
            ${SOURCES_DIR}/mipsopt.c
            ${SOURCES_DIR}/mipsgen.c
            ${SOURCES_DIR}/main.c
            ${SOURCES_DIR}/import.c
            ${SOURCES_DIR}/extdecl.c
            ${SOURCES_DIR}/error.c
            ${SOURCES_DIR}/codes.c
            ${SOURCES_DIR}/codegen.c
            ${SOURCES_DIR}/threads.c)

set(HEADERS ${HEADERS_DIR}/global_vars.h
            ${HEADERS_DIR}/Defs.h
            ${HEADERS_DIR}/th_static.h)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${EXECUTABLE_NAME} m pthread)

# Put the keywords.txt file near the driver
file(GENERATE OUTPUT $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/keywords.txt
              INPUT ${PROJECT_SOURCE_DIR}/keywords.txt)

enable_testing()

set(TEST_TIMEOUT 1)

file(GLOB TEST_INPUTS "${TEST_INPUTS_DIR}/*.c"
                      "${TEST_INPUTS_DIR}/*/*.c"
                      "${TEST_INPUTS_DIR}/*/*/*.c")

foreach(TEST_INPUT ${TEST_INPUTS})
    add_test(NAME ${TEST_INPUT}
             COMMAND "$<TARGET_FILE:${EXECUTABLE_NAME}>" ${TEST_INPUT})
    set_tests_properties(${TEST_INPUT} PROPERTIES TIMEOUT ${TEST_TIMEOUT})
endforeach(TEST_INPUT)
