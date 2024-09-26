# Update history of SHAPELIB copy

* files `shpopen.c`, `shapefil.h`, `dbfopen.c`, `shapefil_private.h`
   from GDAL [ogr/ogrsf_frmts/shape/](https://github.com/OSGeo/gdal/tree/master/ogr/ogrsf_frmts/shape)
* file `safileio.c`
   from [SHAPELIB](http://download.osgeo.org/shapelib/)

## Last update

* taken from GDAL 3.9.2 and SHAPELIB 1.6.0 (Sep 2024)
* taken from GDAL 3.5.3 and SHAPELIB 1.5.0 (Dec 2022)
* taken from GDAL 2.1.2 and SHAPELIB 1.3.0 (Thu Nov 24 10:45:41 CET 2016)
* taken from GDAL 1.5.1-SVN (Sun Mar 30 11:20:43 CEST 2008)
* taken from GDAL 1.5.0-CVS (Wed Sep  5 13:48:48 CEST 2007)
* taken from GDAL 1.3.2-CVS (Sat Jun 17 22:08:04 CEST 2006)

## Summary of fixes

* dbfopen.c
   around line 1229: GDAL bug [ticket-#809](http://trac.osgeo.org/gdal/ticket/809)
* safileio.c
   [shapelib commit 316ff87](https://github.com/OSGeo/shapelib/commit/316ff872566ea0d91d6b62fe01bfe39931db39aa#diff-f068bc465ca1a32e1b9c214d4eb9504ef9e0f3c4cabc1aa4bab8aa41e2248cc6R153)

## Full fix

```diff
diff --git a/lib/external/shapelib/dbfopen.c b/lib/external/shapelib/dbfopen.c
index 5380e3e20b..5151148d33 100644
--- a/lib/external/shapelib/dbfopen.c
+++ b/lib/external/shapelib/dbfopen.c
@@ -1226,9 +1226,10 @@ DBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName,
     else if( psDBF->pachFieldType[iField] == 'N'
              || psDBF->pachFieldType[iField] == 'F' )
     {
-    if( psDBF->panFieldDecimals[iField] > 0
-            || psDBF->panFieldSize[iField] >= 10 )
+    if( psDBF->panFieldDecimals[iField] > 0 ) {
+        /* || psDBF->panFieldSize[iField] >= 10 ) */ /* GDAL bug #809 */
         return( FTDouble );
+    }
     else
         return( FTInteger );
     }

```
