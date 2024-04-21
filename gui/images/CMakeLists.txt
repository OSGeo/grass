file(GLOB GUI_IMAGES "*.png")

set(output_images)
foreach(IMG ${GUI_IMAGES})
  get_filename_component(FILE_NAME ${IMG} NAME)
  add_custom_command(
    OUTPUT ${GISBASE}/gui/images/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${IMG} ${GISBASE}/gui/images/)
  list(APPEND output_images ${GISBASE}/gui/images/${FILE_NAME})
endforeach()

add_custom_target(
  gui_images
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/symbols
          ${GISBASE}/gui/images/symbols
  COMMAND ${CMAKE_COMMAND} -E rm ${GISBASE}/gui/images/symbols/README
  DEPENDS gui_icons ${output_images})

install(FILES ${GUI_IMAGES} DESTINATION gui/images/)
install(DIRECTORY symbols/basic DESTINATION gui/images/symbols/)
install(DIRECTORY symbols/demo DESTINATION gui/images/symbols/)
install(DIRECTORY symbols/extra DESTINATION gui/images/symbols/)
install(DIRECTORY symbols/geology DESTINATION gui/images/symbols/)
install(DIRECTORY symbols/legend DESTINATION gui/images/symbols/)
install(DIRECTORY symbols/n_arrows DESTINATION gui/images/symbols/)
