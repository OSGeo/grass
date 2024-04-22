add_custom_target(
  fonts
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/fonts
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/fonts/[a-z]*.[a-z]* ${GISBASE}/fonts
  COMMAND ${grass_env_command} ${GISBASE}/bin/g.mkfontcap)
