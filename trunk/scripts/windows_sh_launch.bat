REM Unfortunately not translatable messages here

IF "%GRASS_SHELL%"=="" (
    ECHO The GRASS_SHELL environmental variable is not defined.
    ECHO To use Bash/Shell scripts in GRASS GIS,
    ECHO set the variable using something like:
    ECHO SET GRASS_SHELL="C:\path\to\bash.exe"
    EXIT /b
)

@"%GRASS_SHELL%" "SCRIPT_DIR/SCRIPT_NAME.sh" %*
