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

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _POSIX_C_SOURCE=200809L
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
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/no_entry.phase' > '${CMAKE_BINARY_DIR}/ctest_no_entry.log' 2>&1; status=$?; grep -q \"Missing entry block\" '${CMAKE_BINARY_DIR}/ctest_no_entry.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_type_mismatch
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/type_mismatch.phase' > '${CMAKE_BINARY_DIR}/ctest_type_mismatch.log' 2>&1; status=$?; grep -q \"Type mismatch\" '${CMAKE_BINARY_DIR}/ctest_type_mismatch.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_invalid_token_global
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/invalid_token_global.phase' > '${CMAKE_BINARY_DIR}/ctest_invalid_token_global.log' 2>&1; status=$?; grep -q \"Unexpected token at global scope\" '${CMAKE_BINARY_DIR}/ctest_invalid_token_global.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_missing_paren
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/missing_paren.phase' > '${CMAKE_BINARY_DIR}/ctest_missing_paren.log' 2>&1; status=$?; grep -q \"Expected ')'\" '${CMAKE_BINARY_DIR}/ctest_missing_paren.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_missing_expression
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/missing_expression.phase' > '${CMAKE_BINARY_DIR}/ctest_missing_expression.log' 2>&1; status=$?; grep -q \"Expected expression\" '${CMAKE_BINARY_DIR}/ctest_missing_expression.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_undefined_var
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/undefined_var.phase' > '${CMAKE_BINARY_DIR}/ctest_undefined_var.log' 2>&1; status=$?; grep -q \"Variable 'p' is undefined\" '${CMAKE_BINARY_DIR}/ctest_undefined_var.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_wrong_var_init_count
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/wrong_var_init_count.phase' > '${CMAKE_BINARY_DIR}/ctest_wrong_var_init_count.log' 2>&1; status=$?; grep -q \"Variable initialization mismatch\" '${CMAKE_BINARY_DIR}/ctest_wrong_var_init_count.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_multiple_entry
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/multiple_entry.phase' > '${CMAKE_BINARY_DIR}/ctest_multiple_entry.log' 2>&1; status=$?; grep -q \"Duplicate entry block\" '${CMAKE_BINARY_DIR}/ctest_multiple_entry.log' && [ $status -ne 0 ]"
)

add_test(NAME fail_open_string
    COMMAND /bin/sh -c "'${PHASE_EXE}' '${CMAKE_SOURCE_DIR}/tests/open_string.phase' > '${CMAKE_BINARY_DIR}/ctest_open_string.log' 2>&1; status=$?; grep -q \"Unterminated string\" '${CMAKE_BINARY_DIR}/ctest_open_string.log' && [ $status -ne 0 ]"
)
