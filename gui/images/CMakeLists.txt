set(GUI_IMAGES
    grass_form.png
    small_down_arrow.png
    scalebar-fancy.png
    qgis_world.png
    splash_screen.png
    scalebar-simple.png
    small_up_arrow.png
)

# Paths relative to gui/images/symbols
set(GUI_SYMBOLS
    demo/smrk.png
    demo/muchomurka.png
    legend/area.png
    legend/area_curved.png
    legend/line_crooked.png
    legend/line.png
    n_arrows/n_arrow8b.png
    n_arrows/n_arrow3.png
    n_arrows/n_arrow2.png
    n_arrows/n_arrow8a.png
    n_arrows/n_arrow6.png
    n_arrows/n_arrow5.png
    n_arrows/n_arrow4.png
    n_arrows/n_arrow9.png
    n_arrows/basic_compass.png
    n_arrows/n_arrow7b.png
    n_arrows/n_arrow7a.png
    n_arrows/n_arrow1a.png
    n_arrows/n_arrow1b.png
    n_arrows/fancy_compass.png
    basic/pin.png
    basic/x.png
    basic/hexagon.png
    basic/circle.png
    basic/arrow.png
    basic/arrow1.png
    basic/arrow3.png
    basic/triangle.png
    basic/arrow2.png
    basic/point.png
    basic/pushpin.png
    basic/octagon.png
    basic/diamond.png
    basic/cross1.png
    basic/star.png
    basic/cross3.png
    basic/cross2.png
    basic/box.png
    basic/marker.png
    basic/pin_dot.png
    geology/half-arrow_right.png
    geology/strike_parallel.png
    geology/strike_cleavage.png
    geology/strike_box.png
    geology/strike_half-bowtie.png
    geology/circle_cross.png
    geology/strike_line.png
    geology/half-arrow_left.png
    geology/strike_triangle.png
    geology/strike_circle.png
    extra/dim_arrow.png
    extra/offbox_ne.png
    extra/offbox_nw.png
    extra/bridge.png
    extra/half-circle.png
    extra/dive_flag.png
    extra/target.png
    extra/pentagon.png
    extra/ring.png
    extra/offbox_se.png
    extra/airport.png
    extra/alpha_flag.png
    extra/ping.png
    extra/offbox_sw.png
    extra/fiducial.png
    extra/simple_zia.png
    extra/adcp.png
    extra/4pt_star.png
    extra/fish.png
    extra/half-box.png
)

list(TRANSFORM GUI_SYMBOLS PREPEND "symbols/")
set(GUI_RESOURCES ${GUI_IMAGES} ${GUI_SYMBOLS})

set(output_images)

foreach(resource IN LISTS GUI_RESOURCES)
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${resource}")
    set(dst "${RUNTIME_GRASS_GUIRESDIR}/images/${resource}")

    get_filename_component(dst_dir "${dst}" DIRECTORY)

    add_custom_command(
        OUTPUT "${dst}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dst_dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${src}" "${dst}"
        DEPENDS "${src}"
        VERBATIM
    )

    list(APPEND output_images "${dst}")
endforeach()

add_custom_target(gui_images ALL DEPENDS ${output_images})
add_dependencies(gui_images gui_icons)

set_target_properties(gui_images PROPERTIES FOLDER GUI)

install(FILES ${GUI_IMAGES} DESTINATION ${GRASS_INSTALL_GUIDIR}/images)

foreach(symbol IN LISTS GUI_SYMBOLS)
    get_filename_component(symbol_dir "${symbol}" DIRECTORY)

    install(
        FILES "${symbol}"
        DESTINATION "${GRASS_INSTALL_GUIDIR}/images/${symbol_dir}"
    )
endforeach()
