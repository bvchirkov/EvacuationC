add_library(arraylist STATIC
    arraylist.c arraylist.h
    )

target_compile_options(arraylist
    PRIVATE
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-pedantic
        -Wunused-parameter
        -Wno-unused-parameter
     )
