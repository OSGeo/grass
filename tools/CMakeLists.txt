set(py_files g.html2man/g.html2man.py g.html2man/ghtml.py g.html2man/ggroff.py)
foreach(py_file ${py_files})
  get_filename_component(py_file_NAME ${py_file} NAME)
  file(COPY ${py_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  set(copied_file_path ${CMAKE_CURRENT_BINARY_DIR}/${py_file_NAME})
  set(stamp_file_path ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${py_file_NAME}.stamp)
  add_custom_command(
    OUTPUT ${stamp_file_path}
    DEPENDS g.parser
    COMMAND ${PYTHON_EXECUTABLE} -t -m py_compile ${copied_file_path}
    COMMAND ${CMAKE_COMMAND} -E touch  ${stamp_file_path}
    COMMENT "Generating ${copied_file_path}c"
    VERBATIM)
  add_custom_target(tools.${py_file_NAME} ALL
    DEPENDS ${stamp_file_path})

  install(FILES ${copied_file_path} DESTINATION tools)
endforeach()

set(current_time_s_ms_SRCS "timer/main.c")
if(MSVC)
  list(APPEND current_time_s_ms_SRCS "timer/msvc/gettimeofday.c")
endif()
build_program_in_subdir(timer
NAME current_time_s_ms
DEPENDS grass_gis
SOURCES "${current_time_s_ms_SRCS}")

add_executable(g.echo g.echo.c)

install(TARGETS g.echo DESTINATION tools)
