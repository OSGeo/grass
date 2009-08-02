set ICON=%OSGEO4W_ROOT%\apps\grass\grass-7.0.0svn\etc\gui\icons\grass.ico
set BATCH=%OSGEO4W_ROOT%\bin\grass70.bat

textreplace -std -t "%OSGEO4W_ROOT%"\bin\grass70.bat
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\grass-7.0.0svn\etc\fontcap
textreplace -std -t "%OSGEO4W_ROOT%"\apps\grass\bin\grass70

mkdir "%OSGEO4W_STARTMENU%\GRASS GIS"
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS\wxPython.lnk"   "%BATCH%" "-wxpython" \ "wxPython interface" 1 "%ICON%"
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS\TclTk.lnk"      "%BATCH%" "-tcltk" \ "Tcl/Tk interface" 1 "%ICON%"
xxmklink "%OSGEO4W_STARTMENU%\GRASS GIS\Text.lnk"       "%BATCH%" "-text" \ "Text interface" 1 "%ICON%"

xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS (wxpython).lnk" "%BATCH%" "-wxpython" \ "wxPython" 1 "%ICON%"
xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS (TclTk).lnk" "%BATCH%" "-tcltk" \ "Tcl/Tk" 1 "%ICON%"
xxmklink "%ALLUSERSPROFILE%\Desktop\GRASS GIS (Text).lnk" "%BATCH%" "-text" \ "Text interface" 1 "%ICON%"
