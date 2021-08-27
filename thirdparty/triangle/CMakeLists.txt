cmake_minimum_required(VERSION 3.16)

project(triangle LANGUAGES C)

add_library(triangle
    STATIC
        triangle.c triangle.h
)

target_compile_options(triangle
    PRIVATE
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-pedantic
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-maybe-uninitialized
        -Wno-missing-prototypes
)

if(UNIX)
    target_compile_definitions(triangle PRIVATE -DLINUX)
elseif(WIN32)
    target_compile_definitions(triangle PRIVATE -DNO_TIMER -DCPU86)
endif()

# Generate position independent code
set_target_properties(triangle PROPERTIES POSITION_INDEPENDENT_CODE ON)
