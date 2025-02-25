#[[
AUTHOR(S):  Rashad Kanavath <rashad km gmail>
PURPOSE:    Get host system architecuture
COPYRIGHT:  (C) 2020 by the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later
#]]

function(get_host_arch var_arch)
  set(host_arch_value "x86_64") # default value

  if(WIN32)
    if(MSVC)
      execute_process(
        COMMAND ${CMAKE_C_COMPILER}
        ERROR_VARIABLE ev
        OUTPUT_VARIABLE ov
        OUTPUT_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

      if("${ev}" MATCHES "x86")
        set(${host_arch_value} "x86")
      else()
        set(${host_arch_value} "x86_64")
      endif()

    elseif(MINGW)

    else()
      message(FATAL_ERROR "compiler/platform is not supported")
    endif() # if(MSVC)

  elseif(UNIX)
    execute_process(
      COMMAND uname -m
      ERROR_VARIABLE ev
      OUTPUT_VARIABLE ov
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${host_arch_value} "${ov}")
  endif()

  set(${var_arch}
      ${host_arch_value}
      PARENT_SCOPE)
endfunction()
