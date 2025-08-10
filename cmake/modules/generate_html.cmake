macro(generate_html)
  cmake_parse_arguments(PGM "IMG_NOT" "NAME;SOURCEDIR;TARGET" "" ${ARGN})

  if(NOT PGM_NAME)
    message(FATAL_ERROR "NAME in not set")
  endif()

  if(NOT PGM_TARGET)
    message(FATAL_ERROR "TARGET in not set")
  endif()

  if(NOT PGM_SOURCEDIR)
    set(PGM_SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  set(PGM_SOURCE ${PGM_SOURCEDIR}/${PGM_NAME}.html)

  file(
    GLOB IMG_FILES
    LIST_DIRECTORIES FALSE
    ${PGM_SOURCEDIR}/*.png ${PGM_SOURCEDIR}/*.jpg)
  if(IMG_FILES AND NOT PGM_IMG_NOT)
    set(copy_images_command ${CMAKE_COMMAND} -E copy_if_different ${IMG_FILES}
                            ${OUTDIR}/${GRASS_INSTALL_DOCDIR})
    install(FILES ${IMG_FILES} DESTINATION ${GRASS_INSTALL_DOCDIR})
  endif()

  add_custom_command(
    TARGET ${PGM_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PGM_SOURCE}
            ${CMAKE_CURRENT_BINARY_DIR}/${PGM_NAME}.html
    COMMAND ${grass_env_command} ${PYTHON_EXECUTABLE} ${MKHTML_PY} ${PGM_NAME} >
            ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.html
    COMMAND ${copy_images_command}
    COMMAND ${CMAKE_COMMAND} -E remove
            ${CMAKE_CURRENT_BINARY_DIR}/${PGM_NAME}.html
    COMMENT "Creating ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.[html|1]")
  install(FILES ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/${PGM_NAME}.html
          DESTINATION ${GRASS_INSTALL_DOCDIR})
endmacro()
