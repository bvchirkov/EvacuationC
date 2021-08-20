add_library(triangle SHARED
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
     )
