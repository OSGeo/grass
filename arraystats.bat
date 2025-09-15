python C:/opt/grass/python/libgrass_interface_generator/run.py ^
    --cpp "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe -E /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /DGRASS_CMAKE_BUILD=1 /DWIN32 /D_WINDOWS" ^
    --no-embed-preamble ^
    --debug-level 1 ^
    --strip-build-path "C:/opt/grass/build/output/lib/grass85" ^
    -I"C:/Program Files (x86)/GRASS/include" ^
    -I"C:/opt/grass/build/output/lib/grass85/include" ^
    -I"C:/Users/htdun/anaconda3/envs/grass/Library/include" ^
    -lgrass_arraystats ^
    --save-preprocessed-headers arraystats_preprocessed_headers.txt ^
    -o "C:/opt/grass/build/output/lib/grass85/etc/python/grass/lib/arraystats.py" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/arraystats.h" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/defs/arraystats.h" > C:\opt\grass\build\ctypesgen_output.txt 2>&1



python C:/opt/grass/python/libgrass_interface_generator/run.py ^
    --cpp "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe -E /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /DGRASS_CMAKE_BUILD=1 /DWIN32 /D_WINDOWS" ^
    --no-embed-preamble ^
    --strip-build-path "C:/opt/grass/build/output/lib/grass85" ^
    -I"C:/Program Files (x86)/GRASS/include" ^
    -I"C:/opt/grass/build/output/lib/grass85/include" ^
    -I"C:/Users/htdun/anaconda3/envs/grass/Library/include" ^
    -lgrass_arraystats ^
    -o "C:/opt/grass/build/output/lib/grass85/etc/python/grass/lib/arraystats.py" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/arraystats.h" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/defs/arraystats.h" > C:\opt\grass\build\ctypesgen_output.txt 2>&1


python C:/opt/grass/python/libgrass_interface_generator/run.py ^
    --cpp "D:/mingw64/bin/gcc.exe -E" ^
    --no-embed-preamble ^
    --strip-build-path "C:/opt/grass/build/output/lib/grass85" ^
    -I"C:/Program Files (x86)/GRASS/include" ^
    -I"C:/opt/grass/build/output/lib/grass85/include" ^
    -I"C:/Users/htdun/anaconda3/envs/grass/Library/include" ^
    --save-preprocessed-headers arraystats_preprocessed_headers.txt ^
    -lgrass_arraystats ^
    -o "C:/opt/grass/build/output/lib/grass85/etc/python/grass/lib/arraystats.py" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/arraystats.h" ^
    "C:/opt/grass/build/output/lib/grass85/include/grass/defs/arraystats.h"
