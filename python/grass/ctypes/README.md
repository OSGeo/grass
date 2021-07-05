## Notes on ctypesgen

Currently installed version: https://github.com/davidjamesca/ctypesgen/commit/0681f8ef1742206c171d44b7872c700f34ffe044 (3 March 2020)


### How to update ctypesgen version

1. Replace the GRASS directory `python/grass/ctypes/ctypesgen` with the `ctypesgen`
   directory from ctypesgen source directory.
2. Replace `python/grass/ctypes/run.py` with `run.py` from ctypesgen source directory.
3. Apply the patches below.
4. Update this document with info on installed ctypesgen version.
5. If a patch has been addressed upstreams, also remove its section from this document.

### Patches

It is highly encouraged to report [upstreams](https://github.com/davidjamesca/ctypesgen) necessary patches for GRASS.

#### POINTER patch

https://trac.osgeo.org/grass/ticket/2748
https://trac.osgeo.org/grass/ticket/3641

Every generated GRASS library bridge part (gis.py, raster.py etc.) is a standalone
product. E.g. parts of libgis (include/grass/gis.h) used in libraster
(etc/python/grass/lib/raster.py) are also defined in raster.py. This way there
is a definition of `struct_Cell_head` in both gis.py and raster.py -- a situation
ctypes doesn't approve of. This patch seems to fix related errors in GRASS.
Manually remove e.g. the gis.py parts in raster.py and make an "import" also did
the work, but that would be more difficult to implement as part of ctypesgen
file generation.

```diff
--- ctypesgen/printer_python/preamble/3_2.py.orig
+++ ctypesgen/printer_python/preamble/3_2.py
@@ -14,6 +14,24 @@
 del _int_types
 
 
+def POINTER(obj):
+    p = ctypes.POINTER(obj)
+
+    # Convert None to a real NULL pointer to work around bugs
+    # in how ctypes handles None on 64-bit platforms
+    if not isinstance(p.from_param, classmethod):
+
+        def from_param(cls, x):
+            if x is None:
+                return cls()
+            else:
+                return x
+
+        p.from_param = classmethod(from_param)
+
+    return p
+
+
 class UserString:
     def __init__(self, seq):
         if isinstance(seq, bytes):

```

#### Loader and preamble patch

Replaces sed introduced with:
"Move ctypesgen boilerplate to common module"

https://github.com/OSGeo/grass/commit/59eeff479cd39fd503e276d164977648938cc85b


```diff
--- ctypesgen/printer_python/printer.py.orig
+++ ctypesgen/printer_python/printer.py
@@ -156,19 +156,22 @@
         path, v = get_preamble(**m.groupdict())
 
         self.file.write("# Begin preamble for Python v{}\n\n".format(v))
-        preamble_file = open(path, "r")
-        self.file.write(preamble_file.read())
-        preamble_file.close()
+        self.file.write("from .ctypes_preamble import *\n")
+        self.file.write("from .ctypes_preamble import _variadic_function\n")
+        # preamble_file = open(path, "r")
+        # self.file.write(preamble_file.read())
+        # preamble_file.close()
         self.file.write("\n# End preamble\n")
 
     def print_loader(self):
         self.file.write("_libs = {}\n")
         self.file.write("_libdirs = %s\n\n" % self.options.compile_libdirs)
         self.file.write("# Begin loader\n\n")
-        path = path_to_local_file("libraryloader.py", libraryloader)
-        loader_file = open(path, "r")
-        self.file.write(loader_file.read())
-        loader_file.close()
+        self.file.write("from .ctypes_loader import *\n")        
+        # path = path_to_local_file("libraryloader.py", libraryloader)
+        # loader_file = open(path, "r")
+        # self.file.write(loader_file.read())
+        # loader_file.close()
         self.file.write("\n# End loader\n\n")
         self.file.write(
             "add_library_search_dirs([%s])"

```

#### Mac specific patches

Enable Ctypesgen parsing of non-utf8 files on macOS https://github.com/OSGeo/grass/pull/385

```diff
--- ctypesgen/parser/preprocessor.py.orig
+++ ctypesgen/parser/preprocessor.py
@@ -159,10 +159,29 @@
 
         self.cparser.handle_status(cmd)
 
-        pp = subprocess.Popen(
-            cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
-        )
-        ppout, pperr = pp.communicate()
+        pp = subprocess.Popen(cmd,
+                              shell=True,
+                              universal_newlines=True,
+                              stdout=subprocess.PIPE,
+                              stderr=subprocess.PIPE)
+        try:
+            ppout, pperr = pp.communicate()
+        except UnicodeError:
+            # Fix for https://trac.osgeo.org/grass/ticket/3883,
+            # handling file(s) encoded with mac_roman
+            if sys.platform == 'darwin':
+                pp = subprocess.Popen(cmd,
+                                      shell=True,
+                                      universal_newlines=False,  # read as binary
+                                      stdout=subprocess.PIPE,
+                                      stderr=subprocess.PIPE)
+                ppout, pperr = pp.communicate()
+
+                data = ppout.decode('utf8', errors='replace')
+                ppout = data.replace('\r\n', '\n').replace('\r', '\n')
+                pperr = pperr.decode('utf8', errors='replace')
+            else:
+                raise UnicodeError
 
         for line in pperr.split("\n"):
             if line:

```


macOS: use `@rpath` as dynamic linker https://github.com/OSGeo/grass/pull/981

```diff
--- ctypesgen/libraryloader.py.orig
+++ ctypesgen/libraryloader.py
@@ -168,6 +168,7 @@
         dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
         if not dyld_fallback_library_path:
             dyld_fallback_library_path = [os.path.expanduser("~/lib"), "/usr/local/lib", "/usr/lib"]
+        dyld_fallback_library_path.extend(_environ_path('LD_RUN_PATH'))
 
         dirs = []

``
