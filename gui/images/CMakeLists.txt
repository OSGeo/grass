file(GLOB GUI_IMAGES "*.png")

set(output_images)
foreach(IMG ${GUI_IMAGES})
  get_filename_component(FILE_NAME ${IMG} NAME)
  add_custom_command(
    OUTPUT ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images/${FILE_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${IMG}
            ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images)
  list(APPEND output_images
       ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images/${FILE_NAME})
endforeach()

if(CMAKE_VERSION VERSION_LESS 3.17)
  set(RM remove)
else()
  set(RM rm)
endif()

add_custom_target(
  gui_images
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/symbols
          ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images/symbols
  COMMAND ${CMAKE_COMMAND} -E ${RM}
          ${OUTDIR}/${GRASS_INSTALL_GUIDIR}/images/symbols/README
  DEPENDS gui_icons ${output_images})

install(FILES ${GUI_IMAGES} DESTINATION ${GRASS_INSTALL_GUIDIR}/images)
install(DIRECTORY symbols/basic
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
install(DIRECTORY symbols/demo
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
install(DIRECTORY symbols/extra
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
install(DIRECTORY symbols/geology
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
install(DIRECTORY symbols/legend
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
install(DIRECTORY symbols/n_arrows
        DESTINATION ${GRASS_INSTALL_GUIDIR}/images/symbols)
