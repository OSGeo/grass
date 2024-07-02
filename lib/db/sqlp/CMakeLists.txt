flex_target(sqlp.yy.c sqlp.l ${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c)

bison_target(sqlp.tab.c sqlp.y ${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c)

add_flex_bison_dependency(sqlp.yy.c sqlp.tab.c)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c GENERATED)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c GENERATED)

set(sqlp_SRCS alloc.c print.c sql.c ${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c
              ${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c)

build_module(NAME grass_sqlp SOURCES "${sqlp_SRCS}" NO_DOCS)
