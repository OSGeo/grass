file(GLOB SRCICO "*.ico" "*.png")
file(GLOB SRCICONS "grass/*.png")
file(GLOB SRCFLAGS "flags/*.png")

install(FILES ${SRCICO} DESTINATION gui/icons/)
install(FILES ${SRCICONS} DESTINATION gui/icons/grass)
install(FILES ${SRCFLAGS} DESTINATION gui/icons/flags)

install(FILES ${SRCICONS} DESTINATION docs/html/icons)

install(FILES grass.desktop DESTINATION share/applications)
foreach(icon_type 8 16 22 24 32 36 40 42 48 64 72 80 96 128 192)
  set(icon_size "${icon_type}x${icon_type}")
  install(FILES grass-${icon_size}.png DESTINATION share/icons/hicolor/${icon_size}/apps)
  
endforeach()

install(FILES grass.svg
  DESTINATION share/icons/hicolor/scalable/apps
  RENAME grass71.svg)

install(FILES grass.appdata.xml DESTINATION share/appdata )
