macro(check_target target_name have_define_var)
  set(${have_define_var} 0)
  if(TARGET ${target_name})
    # message(STATUS "${target_name} package found. Setting ${have_define_var}
    # to 1")
    set(${have_define_var} 1)
  endif()
endmacro()
