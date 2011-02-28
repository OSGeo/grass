set ICON=%OSGEO4W_ROOT%\apps\grass\grass-@VERSION@\etc\gui\icons\grass.ico
set BATCH=%OSGEO4W_ROOT%\bin\grass@POSTFIX@.bat

textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass@POSTFIX@-env.bat 
textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass@POSTFIX@.bat 
textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass@POSTFIX@ 
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\grass-@VERSION@\etc\fontcap 

mkdir "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@" 
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\wxPython @VERSION@.lnk"   "%BATCH%" "-wxpython" \ "wxPython interface" 1 "%ICON%" 
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS @VERSION@\Text @VERSION@.lnk"       "%BATCH%" "-text" \ "Text interface" 1 "%ICON%" 

xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@ (wxpython).lnk" "%BATCH%" "-wxpython" \ "wxPython" 1 "%ICON%" 
xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS @VERSION@ (Text).lnk" "%BATCH%" "-text" \ "Text interface" 1 "%ICON%" 
