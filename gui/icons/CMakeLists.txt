file(GLOB GUI_ICONS "*.ico" "*.png")
file(GLOB GRASS_ICONS "grass/*.png")
file(GLOB FLAGS_ICONS "flags/*.png")

add_custom_target(
  make_gui_icons_dirs
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/icons/grass
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/icons/flags
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/docs/html/icons
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GISBASE}/gui/images/symbols)

set(output_icons)
foreach(ICON ${GUI_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${GISBASE}/gui/icons/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON} ${GISBASE}/gui/icons/)
  list(APPEND output_icons ${GISBASE}/gui/icons/${FILE_NAME})
endforeach()

foreach(ICON ${GRASS_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${GISBASE}/gui/icons/grass/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON} ${GISBASE}/gui/icons/grass/)
  list(APPEND output_icons ${GISBASE}/gui/icons/grass/${FILE_NAME})

  add_custom_command(
    OUTPUT ${GISBASE}/docs/html/icons/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON} ${GISBASE}/docs/html/icons/)
  list(APPEND output_icons ${GISBASE}/docs/html/icons/${FILE_NAME})
endforeach()

foreach(ICON ${FLAGS_ICONS})
  get_filename_component(FILE_NAME ${ICON} NAME)
  add_custom_command(
    OUTPUT ${GISBASE}/gui/icons/flags/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ICON} ${GISBASE}/gui/icons/flags/)
  list(APPEND output_icons ${GISBASE}/gui/icons/flags/${FILE_NAME})
endforeach()

add_custom_target(gui_icons DEPENDS make_gui_icons_dirs ${output_icons})

install(FILES ${GUI_ICONS} DESTINATION gui/icons)
install(FILES ${GRASS_ICONS} DESTINATION gui/icons/grass)
install(FILES ${FLAGS_ICONS} DESTINATION gui/icons/flags)
install(FILES ${GRASS_ICONS} DESTINATION docs/html/icons)
install(FILES grass.desktop DESTINATION share/applications)
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
  install(FILES grass-${icon_size}.png
          DESTINATION share/icons/hicolor/${icon_size}/apps)
endforeach()

install(
  FILES grass.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}.svg)

install(FILES grass.appdata.xml DESTINATION share/appdata)
