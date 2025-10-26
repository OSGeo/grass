#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Read git status of grass repository if building from git clone
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

function(repo_status repo_dir)
  set(time_format "%Y-%m-%dT%H:%M:%S+00:00")
  set(GRASS_VERSION_GIT
      "exported"
      PARENT_SCOPE)
  set(GRASS_HEADERS_GIT_HASH
      ${GRASS_VERSION_NUMBER}
      PARENT_SCOPE)
  string(TIMESTAMP GRASS_HEADERS_GIT_DATE ${time_format} UTC)
  set(GRASS_HEADERS_GIT_DATE
      ${GRASS_HEADERS_GIT_DATE}
      PARENT_SCOPE)

  if(NOT EXISTS "${repo_dir}/.git")
    return()
  endif()
  find_package(Git)
  if(NOT GIT_FOUND)
    return()
  endif()

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_rev_OV
    ERROR_VARIABLE git_rev_EV
    RESULT_VARIABLE git_rev_RV)

  if(git_rev_RV)
    message(FATAL_ERROR "Error running git ${git_rev_EV}")
  else()
    string(STRIP ${git_rev_OV} GRASS_VERSION_GIT)
    set(GRASS_VERSION_GIT
        "${GRASS_VERSION_GIT}"
        PARENT_SCOPE)
  endif()

  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h --
            "${CMAKE_SOURCE_DIR}/include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_hash_OV
    ERROR_VARIABLE git_hash_EV
    RESULT_VARIABLE git_hash_RV)

  if(git_hash_RV)
    message(FATAL_ERROR "Error running git ${git_hash_EV}")
  else()
    string(STRIP ${git_hash_OV} GRASS_HEADERS_GIT_HASH)
    set(GRASS_HEADERS_GIT_HASH
        "${GRASS_HEADERS_GIT_HASH}"
        PARENT_SCOPE)
  endif()

  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%ct --
            "${CMAKE_SOURCE_DIR}/include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_date_OV
    ERROR_VARIABLE git_date_EV
    RESULT_VARIABLE git_date_RV)

  if(git_date_RV)
    message(FATAL_ERROR "Error running git ${git_date_EV}")
  else()
    set(ENV{SOURCE_DATE_EPOCH} ${git_date_OV})
    string(TIMESTAMP GRASS_HEADERS_GIT_DATE ${time_format} UTC)
    set(GRASS_HEADERS_GIT_DATE
        "${GRASS_HEADERS_GIT_DATE}"
        PARENT_SCOPE)
  endif()
endfunction()
