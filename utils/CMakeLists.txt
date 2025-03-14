file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkmarkdown.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mkrest.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/thumbnails.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/generate_last_commit_file.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(current_time_s_ms_SRCS "timer/main.c")
if(MSVC)
  list(APPEND current_time_s_ms_SRCS "../msvc/gettimeofday.c")
endif()
build_program_in_subdir(
  timer
  NAME
  current_time_s_ms
  DEPENDS
  grass_gis
  SOURCES
  "${current_time_s_ms_SRCS}"
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_ETCBINDIR}
  NO_DOCS)

build_program(
  NAME
  g.echo
  SOURCES
  g.echo.c
  RUNTIME_OUTPUT_DIR
  ${GRASS_INSTALL_UTILSDIR}
  PACKAGE
  "NONE"
  NO_DOCS)
add_dependencies(g.echo python_doc_utils)
add_custom_target(
  python_doc_utils
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/g.html2man.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_last_commit_file.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/g.html2man.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ggroff.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/g.html2man/ghtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mkhtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mkmarkdown.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mkrest.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR})

install(
  FILES ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/g.html2man.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/generate_last_commit_file.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/ggroff.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/g.html2man.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/ghtml.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkhtml.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkdocs.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkmarkdown.py
        ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkrest.py
  DESTINATION ${GRASS_INSTALL_UTILSDIR})
