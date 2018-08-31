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

/* Documentation:
 * http://www.opendwg.org
 * -> OpenDWG Toolkit Reference
 *
 * Unsupported entities must be added in wrentity()
 *
 * TODO: 3rd dimension is not functional for CIRCLE and ARC
 *       -> required updated of transformation in INSERT
 *          (how to do that??)
 */

#define AD_PROTOTYPES
#define AD_VM_PC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include "ad2.h"
#include "global.h"

#define exampleprintf printf
#define LOCPI M_PI

char buf[1000];
char buf2[1000];

void getEntTypeName(PAD_ENT_HDR adenhd, char *name)
{
    switch (adenhd->enttype) {
    case AD_ENT_LINE:
	strcpy(name, "LINE");
	break;
    case AD_ENT_POINT:
	strcpy(name, "POINT");
	break;
    case AD_ENT_CIRCLE:
	strcpy(name, "CIRCLE");
	break;
    case AD_ENT_SHAPE:
	strcpy(name, "SHAPE");
	break;
    case AD_ENT_ELLIPSE:
	strcpy(name, "ELLIPSE");
	break;
    case AD_ENT_SPLINE:
	strcpy(name, "SPLINE");
	break;
    case AD_ENT_TEXT:
	strcpy(name, "TEXT");
	break;
    case AD_ENT_ARC:
	strcpy(name, "ARC");
	break;
    case AD_ENT_TRACE:
	strcpy(name, "TRACE");
	break;
    case AD_ENT_SOLID:
	strcpy(name, "SOLID");
	break;
    case AD_ENT_BLOCK:
	strcpy(name, "BLOCK");
	break;
    case AD_ENT_ENDBLK:
	strcpy(name, "ENDBLK");
	break;
    case AD_ENT_INSERT:
	strcpy(name, "INSERT");
	break;
    case AD_ENT_ATTDEF:
	strcpy(name, "ATTDEF");
	break;
    case AD_ENT_ATTRIB:
	strcpy(name, "ATTRIB");
	break;
    case AD_ENT_SEQEND:
	strcpy(name, "SEQEND");
	break;
    case AD_ENT_POLYLINE:
	strcpy(name, "POLYLINE");
	break;
    case AD_ENT_VERTEX:
	strcpy(name, "VERTEX");
	break;
    case AD_ENT_LINE3D:
	strcpy(name, "3DLINE");
	break;
    case AD_ENT_FACE3D:
	strcpy(name, "3DFACE");
	break;
    case AD_ENT_DIMENSION:
	strcpy(name, "DIMENSION");
	break;
    case AD_ENT_VIEWPORT:
	strcpy(name, "VIEWPORT");
	break;
    case AD_ENT_SOLID3D:
	strcpy(name, "SOLID3D");
	break;
    case AD_ENT_RAY:
	strcpy(name, "RAY");
	break;
    case AD_ENT_XLINE:
	strcpy(name, "XLINE");
	break;
    case AD_ENT_MTEXT:
	strcpy(name, "MTEXT");
	break;
    case AD_ENT_LEADER:
	strcpy(name, "LEADER");
	break;
    case AD_ENT_TOLERANCE:
	strcpy(name, "TOLERANCE");
	break;
    case AD_ENT_MLINE:
	strcpy(name, "MLINE");
	break;
    case AD_ENT_BODY:
	strcpy(name, "BODY");
	break;
    case AD_ENT_REGION:
	strcpy(name, "REGION");
	break;
    default:
	if (adenhd->enttype == adOle2frameEnttype(dwghandle))
	    strcpy(name, "OLE2FRAME");
	else if (adenhd->enttype == adLwplineEnttype(dwghandle))
	    strcpy(name, "LWPOLYLINE");
	else if (adenhd->enttype == adHatchEnttype(dwghandle))
	    strcpy(name, "HATCH");
	else if (adenhd->enttype == adImageEnttype(dwghandle))
	    strcpy(name, "IMAGE");
	else if (adenhd->enttype == adArcAlignedTextEnttype(dwghandle))
	    strcpy(name, "ArcAlignedText");
	else if (adenhd->enttype == adWipeoutEnttype(dwghandle))
	    strcpy(name, "Wipeout");
	else if (adenhd->enttype == adRtextEnttype(dwghandle))
	    strcpy(name, "Rtext");
	else {			/* regular proxy */

	    G_debug(3, "adenhd->enttype: %d", adenhd->enttype);
	    strcpy(name, "Proxy");
	}
	break;
    }
}

int write_line(PAD_ENT_HDR adenhd, int type, int level)
{
    int i, l;
    double x, y, z, r, ang;

    adSeekLayer(dwghandle, adenhd->entlayerobjhandle, Layer);

    /* Transformation, go up through all levels of transformation */
    /* not sure what is the right order of transformation */
    for (l = level; l >= 0; l--) {
	for (i = 0; i < Points->n_points; i++) {
	    /* scale */
	    x = Points->x[i] * Trans[l].xscale;
	    y = Points->y[i] * Trans[l].yscale;
	    z = Points->z[i] * Trans[l].zscale;
	    /* rotate */
	    r = sqrt(x * x + y * y);
	    ang = atan2(y, x) + Trans[l].rotang;
	    x = r * cos(ang);
	    y = r * sin(ang);
	    /* move */
	    x += Trans[l].dx;
	    y += Trans[l].dy;
	    z += Trans[l].dz;
	    Points->x[i] = x;
	    Points->y[i] = y;
	    Points->z[i] = z;
	}
    }

    Vect_reset_cats(Cats);
    Vect_cat_set(Cats, 1, cat);
    Vect_write_line(&Map, type, Points, Cats);

    /* Cat */
    sprintf(buf, "insert into %s values ( %d", Fi->table, cat);
    db_set_string(&sql, buf);

    /* Entity name */
    getEntTypeName(adenhd, buf2);
    sprintf(buf, ", '%s'", buf2);
    db_append_string(&sql, buf);

    /* Color */
    sprintf(buf, ", %d", adenhd->entcolor);
    db_append_string(&sql, buf);

    /* Weight */
    sprintf(buf, ", %d", adenhd->lineweight);
    db_append_string(&sql, buf);

    /* Layer name */
    if (!Layer->purgedflag && Layer->name != NULL) {
	db_set_string(&str, Layer->name);
	db_double_quote_string(&str);
	sprintf(buf, ", '%s'", db_get_string(&str));
    }
    else {
	sprintf(buf, ", ''");
    }
    db_append_string(&sql, buf);

    /* Block name */
    if (Block != NULL) {
	db_set_string(&str, Block);
	db_double_quote_string(&str);
    }
    else {
	db_set_string(&str, "");
    }
    sprintf(buf, ", '%s'", db_get_string(&str));
    db_append_string(&sql, buf);

    /* Text */
    if (Txt != NULL) {
	db_set_string(&str, Txt);
	db_double_quote_string(&str);
    }
    else {
	db_set_string(&str, "");
    }
    sprintf(buf, ", '%s'", db_get_string(&str));
    db_append_string(&sql, buf);

    db_append_string(&sql, ")");
    G_debug(3, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error("Cannot insert new row: %s", db_get_string(&sql));
    }

    cat++;
    return 0;
}

/* Returns 1 if element has geometry and may be written to vector */
int is_low_level(PAD_ENT_HDR adenhd)
{
    if (adenhd->enttype == AD_ENT_BLOCK || adenhd->enttype == AD_ENT_ENDBLK ||
	adenhd->enttype == AD_ENT_SEQEND || adenhd->enttype == AD_ENT_INSERT)
    {
	return 0;
    }
    return 1;
}

void wrentity(PAD_ENT_HDR adenhd, PAD_ENT aden, int level, AD_VMADDR entlist,
	      int circle_as_point)
{
    short ret;
    PAD_BLOB_CTRL bcptr;
    PAD_ENT_HDR adenhd2;
    PAD_ENT aden2;
    OdaLong il;
    double tempdouble[3], tempbulge, tempwidth[3];
    double x, y, z, ang;
    PAD_BLKH adblkh;
    int layer_found = 1;

    if (is_low_level(adenhd))
	n_elements++;

    /* Check layer name */
    if (layers_opt->answers) {
	int i = 0;

	adSeekLayer(dwghandle, adenhd->entlayerobjhandle, Layer);

	layer_found = 0;
	if (!Layer->purgedflag) {
	    while (layers_opt->answers[i]) {
		if (strcmp(Layer->name, layers_opt->answers[i]) == 0) {
		    layer_found = 1;
		    break;
		}
		i++;
	    }
	}

	if ((!invert_flag->answer && !layer_found) ||
	    (invert_flag->answer && layer_found)) {
	    if (is_low_level(adenhd))
		n_skipped++;
	    if (adenhd->enttype != AD_ENT_INSERT &&
		adenhd->enttype != AD_ENT_POLYLINE)
		return;
	}
    }

    getEntTypeName(adenhd, buf);
    G_debug(1, "Entity: %s", buf);

    Txt = NULL;
    adenhd2 = (PAD_ENT_HDR) G_malloc(sizeof(AD_ENT_HDR));
    aden2 = (PAD_ENT) G_malloc(sizeof(AD_ENT));
    adblkh = (PAD_BLKH) G_malloc(sizeof(AD_BLKH));
    Vect_reset_line(Points);

    /* Check space for lower level */
    if (level + 1 == atrans) {
	atrans += 10;
	Trans = (TRANS *) G_realloc(Trans, atrans * sizeof(TRANS));
    }

    switch (adenhd->enttype) {
    case AD_ENT_LINE:
	Vect_append_point(Points, aden->line.pt0[0], aden->line.pt0[1],
			  aden->line.pt0[2]);
	Vect_append_point(Points, aden->line.pt1[0], aden->line.pt1[1],
			  aden->line.pt1[2]);
	write_line(adenhd, GV_LINE, level);
	break;

    case AD_ENT_FACE3D:
	Vect_append_point(Points, aden->face3d.pt0[0], aden->face3d.pt0[1],
			  aden->face3d.pt0[2]);
	Vect_append_point(Points, aden->face3d.pt1[0], aden->face3d.pt1[1],
			  aden->face3d.pt1[2]);
	Vect_append_point(Points, aden->face3d.pt2[0], aden->face3d.pt2[1],
			  aden->face3d.pt2[2]);
	Vect_append_point(Points, aden->face3d.pt3[0], aden->face3d.pt3[1],
			  aden->face3d.pt3[2]);
	write_line(adenhd, GV_FACE, level);
	break;

    case AD_ENT_SOLID:
	Vect_append_point(Points, aden->solid.pt0[0], aden->solid.pt0[1],
			  aden->solid.pt0[2]);
	Vect_append_point(Points, aden->solid.pt1[0], aden->solid.pt1[1],
			  aden->solid.pt1[2]);
	Vect_append_point(Points, aden->solid.pt2[0], aden->solid.pt2[1],
			  aden->solid.pt2[2]);
	Vect_append_point(Points, aden->solid.pt3[0], aden->solid.pt3[1],
			  aden->solid.pt3[2]);
	write_line(adenhd, GV_FACE, level);
	break;

    case AD_ENT_TEXT:
	Txt = aden->text.textstr;
	Vect_append_point(Points, aden->text.pt0[0], aden->text.pt0[1],
			  aden->line.pt0[2]);
	write_line(adenhd, GV_POINT, level);
	break;


    case AD_ENT_POINT:
	Vect_append_point(Points, aden->point.pt0[0], aden->point.pt0[1],
			  aden->line.pt0[2]);
	write_line(adenhd, GV_POINT, level);
	break;

    case AD_ENT_ARC:
	for (ang = aden->arc.stang; ang < aden->arc.endang;
	     ang += 2 * LOCPI / 360) {
	    x = aden->arc.pt0[0] + aden->arc.radius * cos(ang);
	    y = aden->arc.pt0[1] + aden->arc.radius * sin(ang);
	    z = aden->arc.pt0[2];
	    Vect_append_point(Points, x, y, z);
	}
	x = aden->arc.pt0[0] + aden->arc.radius * cos(aden->arc.endang);
	y = aden->arc.pt0[1] + aden->arc.radius * sin(aden->arc.endang);
	z = aden->arc.pt0[2];
	Vect_append_point(Points, x, y, z);
	write_line(adenhd, GV_LINE, level);
	break;

    case AD_ENT_CIRCLE:
	if (circle_as_point) {
	    Vect_append_point(Points, aden->circle.pt0[0],
			      aden->circle.pt0[1], aden->circle.pt0[3]);
	    write_line(adenhd, GV_POINT, level);
	}
	else {
	    for (ang = 0; ang < 2 * LOCPI; ang += 2 * LOCPI / 360) {
		x = aden->circle.pt0[0] + aden->circle.radius * cos(ang);
		y = aden->circle.pt0[1] + aden->circle.radius * sin(ang);
		z = aden->circle.pt0[3];
		Vect_append_point(Points, x, y, z);
	    }
	    Vect_append_point(Points, Points->x[0], Points->y[0],
			      Points->z[0]);
	    write_line(adenhd, GV_LINE, level);
	}
	break;

	/* BLOCK starts block of entities but makes no transformation - is it right ? 
	 *  -> do nothing just warn for xref */
    case AD_ENT_BLOCK:
	if (aden->block.xrefpath[0]) {
	    G_warning
		("External reference for block not supported.\n  xref: %s",
		 aden->block.xrefpath);
	}
	Block = G_store(aden->block.name2);
	break;

    case AD_ENT_ENDBLK:	/* endblk - no data */
	G_free(Block);
	Block = NULL;
	break;

    case AD_ENT_INSERT:	/* insert */
	/* get transformation */
	/* TODO: fix rotation for CIRCLE and ARC */
	G_debug(3, " x,y,z: %f, %f, %f", aden->insert.pt0[0],
		aden->insert.pt0[1], aden->insert.pt0[2]);
	G_debug(3, " xscale, yscale, zscale: %f, %f, %f", aden->insert.xscale,
		aden->insert.yscale, aden->insert.zscale);
	G_debug(3, " rotang: %f", aden->insert.rotang);
	G_debug(3, " ncols, nrows: %d, %d", aden->insert.numcols,
		aden->insert.numrows);
	G_debug(3, " coldist, rowdist: %f, %f", aden->insert.coldist,
		aden->insert.rowdist);

	/* write block entities */
	adSeekBlockheader(dwghandle, aden->insert.blockheaderobjhandle,
			  adblkh);
	if (!adblkh->purgedflag) {
	    adStartEntityGet(adblkh->entitylist);
	    while (1) {
		ret = adGetEntity(adblkh->entitylist, adenhd2, aden2);
		if (adenhd2->enttype == AD_ENT_ENDBLK)
		    break;
		if (ret) {
		    /* Set transformation for lower level */
		    Trans[level + 1].dx = aden->insert.pt0[0];
		    Trans[level + 1].dy = aden->insert.pt0[1];
		    Trans[level + 1].dz = aden->insert.pt0[2];
		    Trans[level + 1].xscale = aden->insert.xscale;
		    Trans[level + 1].yscale = aden->insert.yscale;
		    Trans[level + 1].zscale = aden->insert.zscale;
		    Trans[level + 1].rotang = aden->insert.rotang;
		    wrentity(adenhd2, aden2, level + 1, adblkh->entitylist,
			     circle_as_point);
		}
	    }
	}
	break;

    case AD_ENT_SEQEND:	/* seqend */
	break;

    case AD_ENT_POLYLINE:
	while (1) {
	    ret = adGetEntity(entlist, adenhd2, aden2);
	    if (ret != 1) {
		G_warning("Cannot get entity: %d: %s.", adError(),
			  adErrorStr(adError()));
		break;
	    }

	    if (adenhd2->enttype == AD_ENT_SEQEND)
		break;
	    if (adenhd2->enttype != AD_ENT_VERTEX) {
		getEntTypeName(adenhd2, buf);
		G_warning("Expected VERTEX got %s in POLYLINE -> skip", buf);
	    }
	    else {
		Vect_append_point(Points, aden2->vertex.pt0[0],
				  aden2->vertex.pt0[1], aden2->vertex.pt0[2]);
	    }
	};
	if ((!invert_flag->answer && layer_found) ||
	    (invert_flag->answer && !layer_found))
	    write_line(adenhd, GV_LINE, level);
	break;

    default:
	if (adenhd->enttype == adLwplineEnttype(dwghandle)) {
	    G_debug(3, "Npoints: %ld\n", aden->lwpline.numpoints);
	    bcptr = adStartBlobRead(aden->lwpline.ldblob);
	    for (il = 0; il < aden->lwpline.numpoints; il++) {
		adReadBlob2Double(bcptr, tempdouble);
		Vect_append_point(Points, tempdouble[0], tempdouble[1],
				  tempdouble[2]);
		tempbulge = tempwidth[0] = tempwidth[1] = tempwidth[2] = 0.0;
		if (aden->lwpline.flag & AD_LWPLINE_HAS_BULGES) {
		    adReadBlobDouble(bcptr, &tempbulge);
		}
		if (aden->lwpline.flag & AD_LWPLINE_HAS_WIDTHS) {
		    adReadBlob2Double(bcptr, tempwidth);
		}
	    }
	    G_debug(3, "flag = %d", aden->lwpline.flag);
	    if (aden->lwpline.flag & AD_LWPLINE_IS_CLOSED) {
		G_debug(3, "  -> is closed");
		Vect_append_point(Points, Points->x[0], Points->y[0],
				  Points->z[0]);
	    }
	    write_line(adenhd, GV_LINE, level);
	    adEndBlobRead(bcptr);
	}
	else {
	    getEntTypeName(adenhd, buf);
	    G_warning("%s entity not supported", buf);
	}
	break;

    }				/* end of switch */

    G_free(aden2);
    G_free(adenhd2);
}
