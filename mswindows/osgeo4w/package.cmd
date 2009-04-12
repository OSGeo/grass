path %PATH%;%OSGEO4W_ROOT%\apps\swig
set WXWIN=h:\windows\wxPython-src-2.8.9.1

set GRASS_HOME=%OSGEO4W_ROOT%\apps\grass\grass-6.4.0RC3
set GRASS_INC=%GRASS_HOME%\include
set GRASS_LPATH=%GRASS_HOME%\lib

set WX_INC=%WXWIN%\lib\vc_dll\mswud;%WXWIN%\lib\vc_dll\mswu;%WXWIN%\include
set WXPY_INC=%WXWIN%\wxPython\include 
set WX_LPATH=%OSGEO4W_ROOT%\lib;%WXWIN%\lib\vc_dll

set OLD=%CD%
cd %OLD%\gui\wxpython\nviz
python setup.py build_ext -I %OSGEO4W_ROOT%\include;%GRASS_INC%;%WX_INC% -L %GRASS_LPATH%
copy grass6_wxnviz.py                       %GRASS_HOME%\etc\wxpython\nviz
copy build\lib.win32-2.5\_grass6_wxnviz.pyd %GRASS_HOME%\etc\wxpython\nviz
if exist build\lib.win32-2.5\_grass6_wxnviz.pdb copy build\lib.win32-2.5\_grass6_wxnviz.pdb %GRASS_HOME%\etc\wxpython\nviz

cd %OLD%\gui\wxpython\vdigit
python setup.py build_ext -D WXUSINGDLL -I %OSGEO4W_ROOT%\include;%GRASS_INC%;%WX_INC%;%WXPY_INC% -L %GRASS_LPATH%;%WX_LPATH%
copy grass6_wxvdigit.py                       %GRASS_HOME%\etc\wxpython\vdigit
copy build\lib.win32-2.5\_grass6_wxvdigit.pyd %GRASS_HOME%\etc\wxpython\vdigit
if exist build\lib.win32-2.5\_grass6_wxvdigit.pdb copy build\lib.win32-2.5\_grass6_wxvdigit.pdb %GRASS_HOME%\etc\wxpython\vdigit

goto skip

cd %OLD%\swig\python
python setup.py build_ext -I %GRASS_INC%;%OSGEO4W_ROOT%\include -L %OSGEO4W_ROOT%\lib;%GRASS_LPATH%
copy grass6_wxvdigit.py %GRASS_HOME%\etc\wxpython\
copy build\lib.win32-2.5\_grass6_wxvdigit.pyd %GRASS_HOME%\etc\wxpython\vdigit

cd %OLD%\swig\python\NumPtr
python setup.py build_ext


:skip

cd %OLD%
