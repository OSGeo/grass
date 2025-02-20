#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Read major, minor patch, date from given version file
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

function(get_versions file_path var_major var_minor var_release var_date)
  set(version_major 0)
  set(version_minor 0)
  set(version_release 0)
  set(version_date 00000000)
  file(STRINGS "${file_path}" grass_version_strings)
  list(LENGTH grass_version_strings grass_version_file_length)

  if(grass_version_file_length LESS 3)
    message(FATAL_ERROR "include/VERSION is not a valid file")
  endif()

  list(GET grass_version_strings 0 version_major)
  list(GET grass_version_strings 1 version_minor)

  if(grass_version_file_length GREATER 2)
    list(GET grass_version_strings 2 version_release)
  endif()

  if(grass_version_file_length GREATER 3)
    list(GET grass_version_strings 3 version_date)
  endif()

  set(${var_major}
      ${version_major}
      PARENT_SCOPE)
  set(${var_minor}
      ${version_minor}
      PARENT_SCOPE)
  set(${var_release}
      ${version_release}
      PARENT_SCOPE)
  set(${var_date}
      ${version_date}
      PARENT_SCOPE)
endfunction()
