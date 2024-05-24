file(GLOB GUI_ICONS "*.ico" "*.png")
file(GLOB GRASS_ICONS "grass/*.png")
file(GLOB FLAGS_ICONS "flags/*.png")

add_custom_target(
  make_gui_icons_dirs
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/grass
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/flags
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/icons
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images/symbols)

set(output_icons)
foreach(ICON ${GUI_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON}
            ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons)
  list(APPEND output_icons ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/${FILE_NAME})
endforeach()

foreach(ICON ${GRASS_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/grass/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON}
            ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/grass)
  list(APPEND output_icons
       ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/grass/${FILE_NAME})

  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/icons/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON}
            ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/icons)
  list(APPEND output_icons ${OUTDIR}/${GRASS_INSTALL_DOCDIR}/icons/${FILE_NAME})
endforeach()

foreach(ICON ${FLAGS_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/flags/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON}
            ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/flags)
  list(APPEND output_icons
       ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/icons/flags/${FILE_NAME})
endforeach()

add_custom_target(gui_icons DEPENDS make_gui_icons_dirs ${output_icons})

install(FILES ${GUI_ICONS} DESTINATION ${GRASS_INSTALL_GUIDIR}/icons)
install(FILES ${GRASS_ICONS} DESTINATION ${GRASS_INSTALL_GUIDIR}/icons/grass)
install(FILES ${FLAGS_ICONS} DESTINATION ${GRASS_INSTALL_GUIDIR}/icons/flags)
install(FILES ${GRASS_ICONS} DESTINATION ${GRASS_INSTALL_DOCDIR}/icons)
install(FILES grass.desktop DESTINATION ${GRASS_INSTALL_SHAREDIR}/applications)
foreach(
  icon_type
  8
  16
  22
  24
  32
  36
  40
  42
  48
  64
  72
  80
  96
  128
  192)
  set(icon_size "${icon_type}x${icon_type}")
  install(
    FILES grass-${icon_size}.png
    DESTINATION ${GRASS_INSTALL_SHAREDIR}/icons/hicolor/${icon_size}/apps
    RENAME grass.svg)
endforeach()

install(FILES grass.svg
        DESTINATION ${GRASS_INSTALL_SHAREDIR}/icons/hicolor/scalable/apps)

install(FILES grass.appdata.xml DESTINATION ${GRASS_INSTALL_SHAREDIR}/metainfo)
