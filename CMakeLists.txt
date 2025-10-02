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
    -Wextra
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
