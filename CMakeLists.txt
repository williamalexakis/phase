cmake_minimum_required(VERSION 3.20)

project(phase LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PHASE_FORMAT "Have support for running clang-format on sources" OFF)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB HDR_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wextra -Wall)

if(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _POSIX_C_SOURCE=200809L)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(PHASE_FORMAT)
    # if formatting and clang format found
    find_program(CLANG_FORMAT_EXE NAMES clang-format)
    if(CLANG_FORMAT_EXE)
        # custom target running on sources
        add_custom_target(format COMMAND ${CLANG_FORMAT_EXE} -i ${SRC_FILES} ${HDR_FILES}
            COMMENT "Running clang-format on source files..."
            VERBATIM)
    else()
        message(WARNING "phase ---- cannot find clang-format. Target 'format' will not be available.")
    endif()
endif()
