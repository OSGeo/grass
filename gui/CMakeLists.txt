add_subdirectory(icons)
add_subdirectory(images)

add_subdirectory(scripts)

add_subdirectory(wxpython)

add_custom_target(
  grass_interface_dtd ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory
          "${OUTDIR}/${GRASS_INSTALL_GUIDIR}/xml"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/xml/grass-interface.dtd"
    ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/xml)
add_dependencies(grass_interface_dtd GUI_WXPYTHON)
install(FILES ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/xml/grass-interface.dtd
        DESTINATION ${GRASS_INSTALL_GUIDIR}/xml)
