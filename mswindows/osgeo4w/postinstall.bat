set ICON=%OSGEO4W_ROOT%\apps\grass\grass-@VERSION@\etc\gui\icons\grass.ico
set BATCH=%OSGEO4W_ROOT%\bin\grass@POSTFIX@.bat
textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass@POSTFIX@.bat
textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass@POSTFIX@ 
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\etc\fontcap 

mkdir "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@" 
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\Graphical Interface.lnk"   "%BATCH%" "-gui" \ "Graphical User Interface (wxGUI)" 1 "%ICON%" 
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\Text Interface.lnk"  "%BATCH%" "-text" \ "Text User Interface (CLI)" 1 "%ICON%" 

xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@.lnk" "%BATCH%"  "-gui" \ "GRASS GUI" 1 "%ICON%" 
