cmake_minimum_required(VERSION 3.16)

project(EvacuationC LANGUAGES C)

option(build_tests "Build all of own tests" OFF)

add_subdirectory(thirdparty/)

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
    src/bim_tools.c         src/bim_tools.h
    src/bim_graph.c         src/bim_graph.h
    src/bim_evac.c          src/bim_evac.h
    src/bim_polygon_tools.c src/bim_polygon_tools.h
    src/bim_json_object.c   src/bim_json_object.h
    src/bim_configure.c     src/bim_configure.h
    src/main.c
    )

if (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            NDEBUG
            BUILD_STATIC_LIBS=OFF
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./thirdparty/
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        logger
        triangle
        arraylist
        json-c
        m
    )

### Test
if(build_tests)
    enable_testing()
    add_subdirectory(test)
endif()
