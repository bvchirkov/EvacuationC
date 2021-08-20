cmake_minimum_required(VERSION 3.16)

project(EvacuationC LANGUAGES C)

add_subdirectory(src/)
add_subdirectory(thirdparty/)
add_subdirectory(test/)
set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER        gcc-10  CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS           "-std=gnu11 -Wall -Wextra -Wparentheses" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG     "-g -O0" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE   "-Ofast" CACHE STRING "" FORCE)

if ( NOT CMAKE_BUILD_TYPE )
   set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)

message(STATUS "[EvacuationC]::CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

add_executable(${PROJECT_NAME}
    src/main.c
    )


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        bim-tools
        bim-json
    )

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ROOT_PATH="${CMAKE_SOURCE_DIR}/res"
    )
