# libgrass interface generator

## Notes on ctypesgen

Currently installed version:
[**v1.1.1**](https://github.com/ctypesgen/ctypesgen/releases/tag/1.1.1)
(19 October 2022)

### How to update ctypesgen version

1. Replace the GRASS directory `python/libgrass_interface_generator/ctypesgen`
   with the `ctypesgen` directory from ctypesgen source directory.
2. Replace `python/grass/ctypes/run.py` with `run.py` from ctypesgen source directory.
3. Apply the patches below.
4. Update this document with info on installed ctypesgen version.
5. If a patch has been addressed upstreams, also remove its section from this document.

### Patches

It is highly encouraged to report [upstreams](https://github.com/ctypesgen/ctypesgen)
necessary patches for GRASS.

#### Add `bool` ctypes type map for C23 support

Add bool to ctypes type map. This change is needed for C23 support, where
_Bool is not necessarily defined by default.

```diff
--- python/libgrass_interface_generator/ctypesgen/ctypedescs.py.orig
+++ python/libgrass_interface_generator/ctypesgen/ctypedescs.py
@@ -58,6 +58,7 @@
     ("__uint64", False, 0): "c_uint64",
     ("__uint64_t", False, 0): "c_uint64",
     ("_Bool", True, 0): "c_bool",
+    ("bool", True, 0): "c_bool",
 }
 
 ctypes_type_map_python_builtin = {
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

Reported with: <https://github.com/OSGeo/grass/pull/2073>

This patch removes the zero bit sized unnamed structure members from the
generated files.

<!-- markdownlint-disable line-length -->
```diff
--- ctypesgen/printer_python/printer.py.orig
+++ ctypesgen/printer_python/printer.py
@@ -22,6 +22,7 @@ class WrapperPrinter:

         self.file = open(outpath, "w") if outpath else sys.stdout
         self.options = options
+        self.has_unnamed_struct_member = False

         if self.options.strip_build_path and self.options.strip_build_path[-1] != os.path.sep:
             self.options.strip_build_path += os.path.sep
@@ -61,9 +62,14 @@ class WrapperPrinter:
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
@@ -254,6 +260,7 @@ class WrapperPrinter:
             mem = list(struct.members[mi])
             if mem[0] is None:
                 while True:
+                    self.has_unnamed_struct_member = True
                     name = "%s%i" % (anon_prefix, n)
                     n += 1
                     if name not in names:
@@ -266,7 +273,10 @@ class WrapperPrinter:

         self.file.write("%s_%s.__slots__ = [\n" % (struct.variety, struct.tag))
         for name, ctype in struct.members:
-            self.file.write("    '%s',\n" % name)
+            skip_unnamed = (
+                "#unnamedbitfield_{0} ".format(struct.tag) if name.startswith(anon_prefix) else ""
+            )
+            self.file.write("    {0}'{1}',\n".format(skip_unnamed, name))
         self.file.write("]\n")

         if len(unnamed_fields) > 0:
@@ -278,9 +288,15 @@ class WrapperPrinter:
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
@@ -481,3 +497,57 @@ class WrapperPrinter:
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
<!-- markdownlint-enable line-length -->

#### Windows specific patches

Patch for OSGeo4W packaging, adapted from
<https://github.com/jef-n/OSGeo4W/blob/master/src/grass/osgeo4w/patch>

```diff
--- ctypesgen/libraryloader.py.orig
+++ ctypesgen/libraryloader.py
@@ -372,6 +372,12 @@ class WindowsLibraryLoader(LibraryLoader):

     name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

+    def __init__(self):
+        super().__init__()
+        for p in os.getenv("PATH").split(";"):
+            if os.path.exists(p) and hasattr(os, "add_dll_directory"):
+                os.add_dll_directory(p)
+
     class Lookup(LibraryLoader.Lookup):
         """Lookup class for Windows libraries..."""

```

Invoke preprocessor via `sh.exe`, workaround to get the -I switches to be recognized.
<https://trac.osgeo.org/grass/ticket/1125#comment:21>

<https://github.com/OSGeo/grass/commit/65eef4767aa416ca55f7e36f62dce7ce083fe450>

```diff
--- ctypesgen/parser/preprocessor.py.orig
+++ ctypesgen/parser/preprocessor.py
@@ -125,6 +125,9 @@ class PreprocessorParser(object):

         self.cparser.handle_status(cmd)

+        if IS_WINDOWS:
+            cmd = ["sh.exe", "-c", cmd]
+
         pp = subprocess.Popen(
             cmd,
             shell=True,

```
