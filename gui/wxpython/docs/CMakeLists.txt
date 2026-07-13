set(doc_files
    wxGUI.components
    wxGUI
    wxGUI.iscatt
    wxGUI.modules
    wxGUI.nviz
    wxGUI.toolboxes
    wxGUI.vnet)

set(img_files
    wxGUI_iscatt.jpg
    wxGUI_layer_manager.png
    wxGUI_map_display.jpg
    wxGUI_modules_flags.png
    wxGUI_modules_parameters.png
    wxGUI_modules_style_left.png
    wxGUI_modules_style_top.png
    wxGUI_modules_widget_file1.png
    wxGUI_modules_widget_file2.png
    wxGUI_modules_widget_selection.png
    wxGUI_nviz_toolbar.jpg
    wxGUI_nviz_tools_light.jpg
    wxGUI_nviz_tools_surface.jpg
    wxGUI_nviz_tools_vector.jpg
    wxGUI_nviz_tools_view.jpg
    wxGUI_nviz_tools_volume.jpg
    wxGUI_toolboxes.jpg
    wxGUI_vnet.jpg)

add_custom_target(wxpython_docs DEPENDS grass_gis LIB_PYTHON)
add_dependencies(GUI_WXPYTHON wxpython_docs)
set_target_properties(wxpython_docs PROPERTIES FOLDER GUI)

generate_docs_list(
  TARGET wxpython_docs
  DOC_FILES ${doc_files}
  IMG_FILES ${img_files})

unset(doc_files)
unset(img_files)
