copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py tools)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/g.html2man.py tools)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ghtml.py tools)
copy_python_file(${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ggroff.py tools)

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

