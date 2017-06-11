set(py_files g.html2man/g.html2man.py g.html2man/html.py g.html2man/groff.py)
foreach(py_file ${py_files})
  get_filename_component(py_file_NAME ${py_file} NAME)


    file(COPY ${py_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    # <----- RESET HERE ----->
    set(py_file ${CMAKE_CURRENT_BINARY_DIR}/${py_file_NAME})
    # <----- RESET HERE ----->


  add_custom_command(
    OUTPUT ${py_file}.stamp
    DEPENDS g.parser
    COMMAND ${PYTHON_EXECUTABLE} -t -3 -m py_compile ${py_file}
    COMMAND ${CMAKE_COMMAND} -E touch  ${py_file}.stamp
    COMMENT "Generating ${py_file}c"
    VERBATIM
    )
  
  add_custom_target(tools.${py_file_NAME} ALL
    DEPENDS ${py_file}.stamp
    )

    
  # add_custom_target(tools_${py_file_NAME} ALL
  #   COMMAND ${PYTHON_EXECUTABLE} -t -3 -m py_compile ${py_file}
  #   COMMENT "Generating ${py_file}c"
  #   )
  
  install(FILES ${py_file} DESTINATION tools )
endforeach()

build_program_in_subdir(timer NAME current_time_s_ms DEPENDS grass_gis )

add_executable(g.echo g.echo.c)

install(TARGETS g.echo DESTINATION tools)
