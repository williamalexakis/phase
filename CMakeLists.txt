cmake_minimum_required(VERSION 3.20)

project(phase VERSION 1.1.2 
    LANGUAGES C
    DESCRIPTION "A statically-typed bytecode-interpreted programming language in C with zero dependencies."
)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PHASE_FORMAT "Have support for running clang-format on sources" OFF)
option(PHASE_TIDY "Have support for running clang-tidy on sources" OFF)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB HDR_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wextra -Wall)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

if(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _POSIX_C_SOURCE=200809L)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "williamalexakis")
set(CPACK_PACKAGE_CONTACT "w.alexakis@icloud.com")
set(CPACK_RESOURCE_PACKAGE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "w.alexakis@icloud.com")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;DragNDrop")
else()
    set(CPACK_GENERATOR "ZIP;TGZ;DEB;RPM")
endif()
include(CPack)

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

if(PHASE_TIDY)
    # if tidying and clang tidy found
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        # custom target running on sources
        add_custom_target(tidy COMMAND ${CLANG_TIDY_EXE} ${SRC_FILES} ${HDR_FILES}
            COMMENT "Running clang-tidy on source files..."
            VERBATIM)
    else()
        message(WARNING "phase ---- cannot find clang-tidy. Target 'tidy' will not be available.")
    endif()
endif()
