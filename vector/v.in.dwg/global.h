
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

/* transformation, first level is 0 ( called from main ) and transformation 
 *  for this level is 0,0,0, 1,1,1, 0 so that no transformation is done on first level
 *  (not efective but better readable?) */
typedef struct
{
    double dx, dy, dz;
    double xscale, yscale, zscale;
    double rotang;
} TRANS;

extern int cat;
extern int n_elements;		/* number of processed elements (only low level elements) */
extern int n_skipped;		/* number of skipped low level elements (different layer name) */
extern struct Map_info Map;
extern dbDriver *driver;
extern dbString sql;
extern dbString str;
extern struct line_pnts *Points;
extern struct line_cats *Cats;
extern PAD_LAY Layer;
extern char *Txt;
extern char *Block;
extern struct field_info *Fi;
extern AD_DB_HANDLE dwghandle;
extern TRANS *Trans;		/* transformation */
extern int atrans;		/* number of allocated levels */
extern struct Option *layers_opt;
extern struct Flag *invert_flag;

void wrentity(PAD_ENT_HDR adenhd, PAD_ENT aden, int level, AD_VMADDR entlist,
	      int circle_as_point);
