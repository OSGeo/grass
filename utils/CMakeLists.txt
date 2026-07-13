# Copy utility script files, fix file permissions
set(py_files
  g.html2man/g.html2man.py
  g.html2man/ggroff.py
  g.html2man/ghtml.py
  generate_last_commit_file.py
  mkdocs.py
  mkhtml.py
  mkmarkdown.py
  thumbnails.py)
foreach(py_file ${py_files})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${py_file} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  cmake_path(GET py_file FILENAME pyfile)
  file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/${pyfile}
       FILE_PERMISSIONS
         OWNER_READ OWNER_WRITE OWNER_EXECUTE
         GROUP_READ GROUP_EXECUTE
         WORLD_READ WORLD_EXECUTE)
endforeach()

set(current_time_s_ms_SRCS "timer/main.c")
if(MSVC)
  list(APPEND current_time_s_ms_SRCS "../msvc/sys/gettimeofday.c")
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
    ${CMAKE_CURRENT_BINARY_DIR}/g.html2man.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/generate_last_commit_file.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/g.html2man.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ggroff.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ghtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/mkhtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/mkdocs.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/mkmarkdown.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR})

set_target_properties(python_doc_utils PROPERTIES FOLDER Docs)

install(
  PROGRAMS
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/g.html2man.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/generate_last_commit_file.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/ggroff.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/ghtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkdocs.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkhtml.py
    ${OUTDIR}/${GRASS_INSTALL_UTILSDIR}/mkmarkdown.py
  DESTINATION
    ${GRASS_INSTALL_UTILSDIR})
