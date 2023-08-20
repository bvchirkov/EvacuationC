cmake_minimum_required(VERSION 3.16)

project(EvacuationC
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Evacuation modeling"
    HOMEPAGE_URL https://github.com/bvchirkov/EvacuationC
)

set(CMAKE_PROJECT_INCLUDE_BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Policy.cmake)

include(ExternalProject)
include(FetchContent)

option(build_tests "Build all of own tests" OFF)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(CMAKE_C_FLAGS           "-std=gnu11 -W -Wall -Wextra -Wparentheses -Wshadow -funsigned-char")
set(CMAKE_C_FLAGS_DEBUG     "-g -O0")
#set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fsanitize=pointer-subtract -fsanitize=shift")
set(CMAKE_C_FLAGS_RELEASE   "-O3 -DNDEBUG")

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(${PROJECT_NAME}
    src/bim_tools.c         src/bim_tools.h
    src/bim_graph.c         src/bim_graph.h
    src/bim_evac.c          src/bim_evac.h
    src/bim_polygon_tools.c src/bim_polygon_tools.h
    src/bim_configure.c     src/bim_configure.h
    src/bim_json_object.c   src/bim_json_object.h
    src/bim_cli.c           src/bim_cli.h
                            src/bim_uuid.h
    src/bim_output.c        src/bim_output.h
    src/main.c
    )

FetchContent_Declare(
    c-logger
    GIT_REPOSITORY https://github.com/NikBel3476/c-logger.git
    GIT_TAG f02747287296b5a0da1cc2b9d7f4e98a3be444d9 # upstream/master
)

FetchContent_Declare(
    json-c
    GIT_REPOSITORY https://github.com/json-c/json-c.git
    GIT_TAG b4c371fa0cbc4dcbaccc359ce9e957a22988fb34 # json-c-0.17-20230812
)

FetchContent_MakeAvailable(c-logger json-c)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./thirdparty/triangle
        ./thirdparty/arraylist
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        logger_static
        triangle
        pthread
        arraylist
        json-c-static
        m
)


add_subdirectory(thirdparty/triangle)
add_subdirectory(thirdparty/arraylist)

### Test
if(build_tests)
    enable_testing()
    add_subdirectory(test)
endif()
