add_library(bim-json SHARED
    bim_json_object.c bim_json_object.h
    )

find_library(JSON-C json-c)
target_link_libraries(bim-json PRIVATE json-c)
