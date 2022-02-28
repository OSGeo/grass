## Notes on ctypesgen

Currently installed version:
https://github.com/ctypesgen/ctypesgen/commit/0681f8ef1742206c171d44b7872c700f34ffe044 (3 March 2020)


### How to update ctypesgen version

1. Replace the GRASS directory `python/libgrass_interface_generator/ctypesgen` with the `ctypesgen`
   directory from ctypesgen source directory.
2. Replace `python/grass/ctypes/run.py` with `run.py` from ctypesgen source directory.
3. Apply the patches below.
4. Update this document with info on installed ctypesgen version.
5. If a patch has been addressed upstreams, also remove its section from this document.

### Patches

It is highly encouraged to report [upstreams](https://github.com/ctypesgen/ctypesgen) necessary patches for GRASS.

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

#### Ctypes "unnamed structure member with 0 bit size"-patch

Using unnamed zero bit sized structure members, e.g.:

```c
struct timespec {
    time_t tv_sec;
    int :8*(sizeof(time_t)-sizeof(long))*(__BYTE_ORDER==4321);
    long tv_nsec;
    int :8*(sizeof(time_t)-sizeof(long))*(__BYTE_ORDER!=4321);
};
```

is not supported by Ctypes. Ctypesgen generates this code to:

```py
struct_timespec._fields_ = [                         
    ('tv_sec', time_t),                           
    ('unnamed_1', c_int, ((8 * (sizeof(time_t) - sizeof(c_long))) * (1234 == 4321))),
    ('tv_nsec', c_long),                                                             
    ('unnamed_2', c_int, ((8 * (sizeof(time_t) - sizeof(c_long))) * (1234 != 4321))),
]  
```

which therefore causes `ValueError: number of bits invalid for bit field`
(if the bit size expression equals 0).

Reported with: https://github.com/OSGeo/grass/pull/2073

This patch removes the zero bit sized unnamed structure members from the
generated files.

```diff
--- ctypesgen/printer_python/printer.py.orig
+++ ctypesgen/printer_python/printer.py
@@ -46,6 +46,7 @@
 
         self.file = open(outpath, "w") if outpath else sys.stdout
         self.options = options
+        self.has_unnamed_struct_member = False
 
         if self.options.strip_build_path and self.options.strip_build_path[-1] != os.path.sep:
             self.options.strip_build_path += os.path.sep
@@ -82,9 +83,14 @@
         self.print_group(self.options.inserted_files, "inserted files", self.insert_file)
         self.strip_prefixes()
 
-    def __del__(self):
+        if self.has_unnamed_struct_member and outpath:
+            self._add_remove_zero_bitfields()
+
         self.file.close()
 
+        if self.has_unnamed_struct_member and outpath and sys.executable:
+            os.system("{0} {1}".format(sys.executable, outpath))
+
     def print_group(self, list, name, function):
         if list:
             self.file.write("# Begin %s\n" % name)
@@ -231,6 +237,7 @@
             mem = list(struct.members[mi])
             if mem[0] is None:
                 while True:
+                    self.has_unnamed_struct_member = True
                     name = "%s%i" % (anon_prefix, n)
                     n += 1
                     if name not in names:
@@ -243,7 +250,10 @@
 
         self.file.write("%s_%s.__slots__ = [\n" % (struct.variety, struct.tag))
         for name, ctype in struct.members:
-            self.file.write("    '%s',\n" % name)
+            skip_unnamed = (
+                "#unnamedbitfield_{0} ".format(struct.tag) if name.startswith(anon_prefix) else ""
+            )
+            self.file.write("    {0}'{1}',\n".format(skip_unnamed, name))
         self.file.write("]\n")
 
         if len(unnamed_fields) > 0:
@@ -255,9 +265,15 @@
         self.file.write("%s_%s._fields_ = [\n" % (struct.variety, struct.tag))
         for name, ctype in struct.members:
             if isinstance(ctype, CtypesBitfield):
+                skip_unnamed = (
+                    "#unnamedbitfield_{0} ".format(struct.tag)
+                    if name.startswith(anon_prefix)
+                    else ""
+                )
                 self.file.write(
-                    "    ('%s', %s, %s),\n"
-                    % (name, ctype.py_string(), ctype.bitfield.py_string(False))
+                    "    {0}('{1}', {2}, {3}),\n".format(
+                        skip_unnamed, name, ctype.py_string(), ctype.bitfield.py_string(False)
+                    )
                 )
             else:
                 self.file.write("    ('%s', %s),\n" % (name, ctype.py_string()))
@@ -458,3 +474,57 @@
         )
 
         inserted_file.close()
+
+    def _add_remove_zero_bitfields(self):
+        self.file.write(
+            "#REMOVE_START\n"
+            "def main():\n"
+            "    zero_bitfield_list = list()\n"
+            "    filename = os.path.abspath(__file__)\n"
+            "\n"
+            '    with open(filename, "r") as f:\n'
+            "        regex = re.compile(\n"
+            r'            r"([\s]*)(\#unnamedbitfield)_"'
+            "\n"
+            r'            r"(?P<struct_name>[a-zA-Z_].[a-zA-Z0-9_]*)\s(?P<expr>.*)\,"'
+            "\n"
+            "        )\n"
+            "        for line in f:\n"
+            "            m = regex.match(line)\n"
+            "            if m:\n"
+            '                struct_name = m.group("struct_name")\n'
+            '                bitfield_expression = tuple(eval(m.group("expr")))\n'
+            "\n"
+            "                if len(bitfield_expression) == 3 and bitfield_expression[2] == 0:\n"
+            "                    member = bitfield_expression[0]\n"
+            "                    zero_bitfield_list.append((struct_name, member))\n"
+            "\n"
+            '    with open(filename, "r+") as f:\n'
+            "        filedata = f.read()\n"
+            "\n"
+            "        for (struct_name, member) in zero_bitfield_list:\n"
+            "            pat = re.compile(\n"
+            r"""                r"( *)#unnamedbitfield_{0}( '| \('){1}.*\n".format("""
+            "\n"
+            "                    struct_name, member\n"
+            "                )\n"
+            "            )\n"
+            '            filedata = pat.sub("", filedata)\n'
+            "\n"
+            r'        regex = re.compile(r"#REMOVE_START.*#REMOVE_END\n", re.DOTALL)'
+            "\n"
+            '        filedata = regex.sub("", filedata)\n'
+            "\n"
+            r"""        regex = re.compile(r"#unnamedbitfield_[^'\(]*")"""
+            "\n"
+            '        filedata = regex.sub("", filedata)\n'
+            "\n"
+            "        f.seek(0)\n"
+            "        f.write(filedata)\n"
+            "        f.truncate()\n"
+            "\n"
+            "\n"
+            'if __name__ == "__main__":\n'
+            "    main()\n"
+            "#REMOVE_END\n"
+        )

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

Enable Ctypesgen parsing of non-utf8 files on macOS
https://github.com/OSGeo/grass/pull/385

```diff
--- ctypesgen/parser/preprocessor.py.orig
+++ ctypesgen/parser/preprocessor.py
@@ -160,9 +160,32 @@
         self.cparser.handle_status(cmd)
 
         pp = subprocess.Popen(
-            cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
+            cmd,
+            shell=True,
+            universal_newlines=True,
+            stdout=subprocess.PIPE,
+            stderr=subprocess.PIPE,
         )
-        ppout, pperr = pp.communicate()
+        try:
+            ppout, pperr = pp.communicate()
+        except UnicodeError:
+            # Fix for https://trac.osgeo.org/grass/ticket/3883,
+            # handling file(s) encoded with mac_roman
+            if sys.platform == "darwin":
+                pp = subprocess.Popen(
+                    cmd,
+                    shell=True,
+                    universal_newlines=False,  # read as binary
+                    stdout=subprocess.PIPE,
+                    stderr=subprocess.PIPE,
+                )
+                ppout, pperr = pp.communicate()
+
+                data = ppout.decode("utf8", errors="replace")
+                ppout = data.replace("\r\n", "\n").replace("\r", "\n")
+                pperr = pperr.decode("utf8", errors="replace")
+            else:
+                raise UnicodeError
 
         for line in pperr.split("\n"):
             if line:

```


macOS: use `@rpath` as dynamic linker
https://github.com/OSGeo/grass/pull/981

```diff
--- ctypesgen/libraryloader.py.orig
+++ ctypesgen/libraryloader.py
@@ -168,6 +168,7 @@
         dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
         if not dyld_fallback_library_path:
             dyld_fallback_library_path = [os.path.expanduser("~/lib"), "/usr/local/lib", "/usr/lib"]
+        dyld_fallback_library_path.extend(_environ_path('LD_RUN_PATH'))
 
         dirs = []

```


#### Windows specific patches

The type `__int64` isn't defined in ctypesgen
https://trac.osgeo.org/grass/ticket/3506

```diff
--- ctypesgen/ctypedescs.py.orig
+++ ctypesgen/ctypedescs.py
@@ -41,6 +41,7 @@ ctypes_type_map = {
     ("int16_t", True, 0): "c_int16",
     ("int32_t", True, 0): "c_int32",
     ("int64_t", True, 0): "c_int64",
+    ("__int64", True, 0): "c_int64",
     ("uint8_t", True, 0): "c_uint8",
     ("uint16_t", True, 0): "c_uint16",
     ("uint32_t", True, 0): "c_uint32",

```

Patch for OSGeo4W packaging, adapted from
https://github.com/jef-n/OSGeo4W/blob/master/src/grass/osgeo4w/patch

```diff
--- ctypesgen/libraryloader.py.orig
+++ ctypesgen/libraryloader.py
@@ -321,6 +321,12 @@
 class WindowsLibraryLoader(LibraryLoader):
     name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]
 
+    def __init__(self):
+        super().__init__()
+        for p in os.getenv("PATH").split(";"):
+            if os.path.exists(p) and hasattr(os, "add_dll_directory"):
+                os.add_dll_directory(p)
+
     class Lookup(LibraryLoader.Lookup):
         def __init__(self, path):
             super(WindowsLibraryLoader.Lookup, self).__init__(path)

```

Invoke preprocessor via `sh.exe`, workaround to get the -I switches to be recognized.
https://trac.osgeo.org/grass/ticket/1125#comment:21

https://github.com/OSGeo/grass/commit/65eef4767aa416ca55f7e36f62dce7ce083fe450

```diff
--- ctypesgen/parser/preprocessor.py.orig
+++ ctypesgen/parser/preprocessor.py
@@ -159,6 +159,9 @@
 
         self.cparser.handle_status(cmd)
 
+        if sys.platform == "win32":
+            cmd = ["sh.exe", "-c", cmd]
+
         pp = subprocess.Popen(
             cmd, shell=True, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
         )

```
