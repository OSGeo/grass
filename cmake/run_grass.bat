@echo off
set MODULE_TOPDIR=@module_top_dir@
set GISBASE=@bin_dir@
set GISRC=@bin_dir@\etc\config\rc
set PATH=@bin_dir@\bin;@bin_dir@\scripts;%PATH%
set LC_ALL=C
set LANG=C
set LANGUAGE=C
set VERSION_NUMBER=@GRASS_VERSION_NUMBER@
set VERSION_DATE=@VERSION_DATE@
%*

