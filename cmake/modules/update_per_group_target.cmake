
function(update_per_group_target t_name)
  if(${t_name} MATCHES "(^([a-z|0-9]+)\\.)")
    string(REPLACE "." ";" name_to_list "${t_name}")
    list(GET name_to_list 0 all_target_name)
    if(all_target_name)
      set(all_target_name ${all_target_name}-all)
    else()
      message(FATAL_ERROR "all_target_name=${all_target_name} \t t_name=${t_name}")      
    endif()
    
    if(TARGET ${all_target_name})
      add_dependencies(${all_target_name} ${t_name})
    else()
      add_custom_target(${all_target_name} DEPENDS ${t_name})
    endif()
  endif()

endfunction()
