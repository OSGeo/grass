# Update history of SHAPELIB copy

* files `shpopen.c`, `shapefil.h`, `dbfopen.c`
   from GDAL [ogr/ogrsf_frmts/shape/](https://github.com/OSGeo/gdal/tree/master/ogr/ogrsf_frmts/shape)
* file `safileio.c`
   from [SHAPELIB](http://download.osgeo.org/shapelib/)

## Last update

* taken from GDAL 3.5.3 and SHAPELIB 1.5.0 (Dec 2022)
* taken from GDAL 2.1.2 and SHAPELIB 1.3.0 (Thu Nov 24 10:45:41 CET 2016)
* taken from GDAL 1.5.1-SVN (Sun Mar 30 11:20:43 CEST 2008)
* taken from GDAL 1.5.0-CVS (Wed Sep  5 13:48:48 CEST 2007)
* taken from GDAL 1.3.2-CVS (Sat Jun 17 22:08:04 CEST 2006)

## Summary of fixes

* dbfopen.c
   around line 1229: GDAL bug [ticket-#809](http://trac.osgeo.org/gdal/ticket/809)

* safileio.c
   SHP_CVSID: ISO C does not allow extra ‘;’ outside of a function

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
diff --git a/lib/external/shapelib/safileio.c b/lib/external/shapelib/safileio.c
index 289d347eaf..7a614a5806 100644
--- a/lib/external/shapelib/safileio.c
+++ b/lib/external/shapelib/safileio.c
@@ -74,7 +74,7 @@
 #include <string.h>
 #include <stdio.h>

-SHP_CVSID("$Id: safileio.c,v 1.6 2018-06-15 19:56:32 erouault Exp $");
+SHP_CVSID("$Id: safileio.c,v 1.6 2018-06-15 19:56:32 erouault Exp $")

 #ifdef SHPAPI_UTF8_HOOKS
 #   ifdef SHPAPI_WINDOWS

```
