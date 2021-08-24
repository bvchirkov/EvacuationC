set(PROJECT_NAME Tests)

add_executable(${PROJECT_NAME}
    test_bim_object.c
    )

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ROOT_PATH="${CMAKE_SOURCE_DIR}/res"
    )

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ../src
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        bim-tools
    )
