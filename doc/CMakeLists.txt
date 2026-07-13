set(doc_files
  grass_database
  projectionintro)

set(img_files
  create_new_project_gui.png
  gi_3dview.jpg
  gi_addons.jpg
  gi_c.jpg
  gi_cartography.jpg
  gi_database.jpg
  gi_display.jpg
  gi_gallery.jpg
  gi_general.jpg
  gi_gui.jpg
  gi_imagery.jpg
  gi_miscellaneous.jpg
  gi_python.jpg
  gi_raster.jpg
  gi_raster3d.jpg
  gi_temporal.jpg
  gi_vector.jpg
  grass_arch.png
  grass_database.png
  jupyter_3d_map.png
  jupyter_interactive_map.png
  jupyter_map.png
  jupyter_series_map.png
  jupyter_timeseries_map.png
  project.png)

add_custom_target(ADD_DOC_DOCS ALL COMMENT "Generate doc/html docs." DEPENDS LIB_PYTHON)
set_target_properties(ADD_DOC_DOCS PROPERTIES FOLDER Docs)

generate_docs_list(
  TARGET ADD_DOC_DOCS
  DOC_FILES ${doc_files}
  IMG_FILES ${img_files})

unset(doc_files)
unset(img_files)
