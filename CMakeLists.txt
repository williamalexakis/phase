cmake_minimum_required(VERSION 3.20)

project(phase LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_C_COMPILER)

    find_program(GCC_COMPILER gcc)

    if(GCC_COMPILER)

        set(CMAKE_C_COMPILER ${GCC_COMPILER})

    endif()

endif()

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB HDR_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE
    -O2
    -Wextra
    -Wall
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

enable_testing()

set(PHASE_EXE $<TARGET_FILE:${PROJECT_NAME}>)

add_test(NAME tokens_helloworld
    COMMAND ${PHASE_EXE} ${CMAKE_SOURCE_DIR}/examples/helloworld.phase --tokens
)
set_tests_properties(tokens_helloworld PROPERTIES
    PASS_REGULAR_EXPRESSION "ENTRY"
)

add_test(NAME ast_helloworld
    COMMAND ${PHASE_EXE} ${CMAKE_SOURCE_DIR}/examples/helloworld.phase --ast
)
set_tests_properties(ast_helloworld PROPERTIES
    PASS_REGULAR_EXPRESSION "PROGRAM"
)

add_test(NAME run_helloworld
    COMMAND ${PHASE_EXE} ${CMAKE_SOURCE_DIR}/examples/helloworld.phase
)
set_tests_properties(run_helloworld PROPERTIES
    PASS_REGULAR_EXPRESSION "Hello world!"
)

add_test(NAME fail_no_entry
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/no_entry.phase' > '${CMAKE_BINARY_DIR}/ctest_no_entry.log' 2>&1; status=$?; grep -q \"No program entrypoint\" '${CMAKE_BINARY_DIR}/ctest_no_entry.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_type_mismatch
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/type_mismatch.phase' > '${CMAKE_BINARY_DIR}/ctest_type_mismatch.log' 2>&1; status=$?; grep -q \"Type mismatch\" '${CMAKE_BINARY_DIR}/ctest_type_mismatch.log' && [ $status -ne 0 ]"
)
