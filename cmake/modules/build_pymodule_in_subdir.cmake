include(copy_python_file)
function(build_pymodule_in_subdir module_name)

  file(GLOB py_files ${module_name}/*.py)
  file(GLOB img_files ${module_name}/*.png  ${module_name}/*.jpg )
  foreach(py_file ${py_files})
     copy_python_file(${py_file} etc/python/grass/${module_name} )
  endforeach()

endfunction()
