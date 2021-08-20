add_subdirectory(bim)

add_library(bim-tools SHARED
    bim_tools.c  bim_tools.h
    bim_graph.c  bim_graph.h
    bim_evac.c   bim_evac.h
    )

target_include_directories(bim-tools
    PUBLIC
        ../thirdparty/
    )

# `m' for math
target_link_libraries(bim-tools
    PRIVATE
        triangle
        arraylist
        m
    )
