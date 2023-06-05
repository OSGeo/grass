file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/generate_last_commit_file.py  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(current_time_s_ms_SRCS "timer/main.c")
if(MSVC)
  list(APPEND current_time_s_ms_SRCS "timer/msvc/gettimeofday.c")
endif()
build_program_in_subdir(timer
NAME current_time_s_ms
DEPENDS grass_gis
SOURCES "${current_time_s_ms_SRCS}"
RUNTIME_OUTPUT_DIR etc)

build_program(NAME g.echo
  SOURCES g.echo.c
  RUNTIME_OUTPUT_DIR tools
  PACKAGE "NONE")
  add_dependencies(g.echo python_doc_tools)
  add_custom_target(python_doc_tools
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py  ${GISBASE}/tools
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/g.html2man.py  ${GISBASE}/tools
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ghtml.py ${GISBASE}/tools
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ggroff.py ${GISBASE}/tools
	)

