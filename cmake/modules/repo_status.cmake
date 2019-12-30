function(repo_status repo_dir version_git_var)

if(NOT EXISTS "${repo_dir}/.git")
   message(STATUS ".git directory not found. GRASS_VERSION_GIT is set to 'exported'")
  set(GRASS_VERSION_GIT "exported")
  return()
endif()
find_package(Git)
if(NOT GIT_FOUND)
  message(WARNING "git not found. GRASS_VERSION_GIT is set to 'exported'")
  set(GRASS_VERSION_GIT "exported")
  return()
endif()

execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_rev_OV
    ERROR_VARIABLE  git_rev_EV
    RESULT_VARIABLE git_rev_RV
    )

  if(git_rev_RV )
    message(FATAL_ERROR "Error running git ${git_rev_EV}")
  else()
    string(STRIP ${git_rev_OV} GRASS_VERSION_GIT)
  endif()

  set(${version_git_var} "${GRASS_VERSION_GIT}" PARENT_SCOPE)

endfunction() #repo_status