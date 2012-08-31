@echo off

setlocal
rem if /i "%1"==path (path %2) && goto:eof

if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\R-core\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
 ) do set R_TOOLS=%%~b
if not defined R_TOOLS for /f "tokens=2*" %%a in (
 'reg query hklm\software\wow6432Node\Rtools /v InstallPath 2^>NUL ^| findstr InstallPath'
  ) do set R_TOOLS=%%~b
if not defined R_TOOLS (
	echo "Warning: Rtools not found in registry."
	if exist "C:\Rtools" set R_TOOLS=C:\Rtools
)
if not defined R_TOOLS (
	echo "Warning: Rtools not found in common locations."
	goto:eof
)

echo Rtools at: %R_TOOLS%
if not exist "%R_TOOLS%\VERSION.txt" (
	echo Rtools version: 2.12 or 2.13 or earlier.
	goto:eof
)

type %R_TOOLS%\VERSION.txt

endlocal

