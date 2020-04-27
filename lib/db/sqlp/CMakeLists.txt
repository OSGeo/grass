
FLEX_TARGET(sqlp.yy.c sqlp.l ${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c)

BISON_TARGET(sqlp.tab.c sqlp.y ${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c )

ADD_FLEX_BISON_DEPENDENCY(sqlp.yy.c sqlp.tab.c)

SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c  GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c GENERATED)

set(sqlp_SRCS
  alloc.c  print.c  sql.c
  ${CMAKE_CURRENT_BINARY_DIR}/sqlp.tab.c
  ${CMAKE_CURRENT_BINARY_DIR}/sqlp.yy.c
  )

build_module(NAME grass_sqlp SOURCES "${sqlp_SRCS}" )
