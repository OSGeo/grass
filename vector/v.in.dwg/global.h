
/* **************************************************************
 * 
 *  MODULE:       v.in.dwg
 *  
 *  AUTHOR(S):    Radim Blazek
 *                
 *  PURPOSE:      Import of DWG/DXF files
 *                
 *  COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * 
 *                This program is free software under the 
 *                GNU General Public License (>=v2). 
 *                Read the file COPYING that comes with GRASS
 *                for details.
 * 
 * In addition, as a special exception, Radim Blazek gives permission
 * to link the code of this program with the OpenDWG libraries (or with
 * modified versions of the OpenDWG libraries that use the same license
 * as OpenDWG libraries), and distribute linked combinations including the two.
 * You must obey the GNU General Public License in all respects for all
 * of the code used other than. If you modify this file, you may extend
 * this exception to your version of the file, but you are not obligated
 * to do so. If you do not wish to do so, delete this exception statement
 * from your version.
 * 
 * **************************************************************/
#ifdef MAIN
#define Global
#else
#define Global extern
#endif

/* transformation, first level is 0 ( called from main ) and transformation 
 *  for this level is 0,0,0, 1,1,1, 0 so that no transformation is done on first level
 *  (not efective but better readable?) */
typedef struct
{
    double dx, dy, dz;
    double xscale, yscale, zscale;
    double rotang;
} TRANS;

Global int cat;
Global int n_elements;		/* number of processed elements (only low level elements) */
Global int n_skipped;		/* number of skipped low level elements (different layer name) */
Global struct Map_info Map;
Global dbDriver *driver;
Global dbString sql;
Global dbString str;
Global struct line_pnts *Points;
Global struct line_cats *Cats;
Global PAD_LAY Layer;
Global char *Txt;
Global char *Block;
Global struct field_info *Fi;
Global AD_DB_HANDLE dwghandle;
Global TRANS *Trans;		/* transformation */
Global int atrans;		/* number of allocated levels */
Global struct Option *layers_opt;
Global struct Flag *invert_flag;

void wrentity(PAD_ENT_HDR adenhd, PAD_ENT aden, int level, AD_VMADDR entlist,
	      int circle_as_point);
