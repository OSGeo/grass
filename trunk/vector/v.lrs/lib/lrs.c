/*
 *  Linear reference system library
 */

 /******************************************************************************
 * Copyright (c) 2004, Radim Blazek (blazek@itc.it)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "lrs.h"

/* threshold used if 2 offsets are checked if identical, necessary because of
 *  representation error */
#define LRS_THRESH 1e-10

typedef struct
{
    int lcat, lid;
    double start_map, end_map;
    double start_mp, start_off;	/* milepost, offset for the beginning of ref. segment */
    double end_mp, end_off;	/* milepost, offset for the end of ref. segment */
} RSEGMENT;

/* Compare 2 segments by start_mp and start_off, used by qsort */
int cmp_rsegment(const void *pa, const void *pb)
{
    RSEGMENT *p1 = (RSEGMENT *) pa;
    RSEGMENT *p2 = (RSEGMENT *) pb;

    return LR_cmp_mileposts(p1->start_mp, p1->start_off, p2->start_mp,
			    p2->start_off);
}

/* For given line cat and offset along the line in the map
 *  find line id and mpost (milepost) + offset in real world in rstable in opened database. 
 *  multip - multiplier specifies number of map units in mpost unit
 *  Returns: 0 - no record found
 *           1 - one record
 *           2 - too many records -> ambiguous 
 */
int LR_get_milepost(dbDriver * driver, char *table_name,
		    char *lcat_col, char *lid_col,
		    char *start_map_col, char *end_map_col,
		    char *start_mp_col, char *start_off_col,
		    char *end_mp_col, char *end_off_col,
		    int line_cat, double map_offset,
		    double multip, int *lid, double *mpost, double *offset)
{
    int i, more, mp;
    char buf[2000];
    double moff, off, soff, roff;
    dbString stmt;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    RSEGMENT *rseg;
    int nrseg;
    double length, map_length, k;

    G_debug(4, "LR_get_milepost() line_cat = %d, map_offset = %f", line_cat,
	    map_offset);
    *lid = 0;
    *mpost = *offset = 0.0;

    /* Because some drivers (dbf) do not support complex queries mixing OR and AND
     *  more records with simple condition are selected and processed here */
    sprintf(buf, "select %s, %s, %s, %s, %s, %s, %s from %s where " "%s = %d and "	/* lcat_col = line_cat */
	    "%s <= %f and "	/* start_map_col <= map_offset */
	    "%s >= %f",		/* end_map_col >= map_offset */
	    lid_col, start_map_col, end_map_col,
	    start_mp_col, start_off_col, end_mp_col, end_off_col,
	    table_name,
	    lcat_col, line_cat,
	    start_map_col, map_offset, end_map_col, map_offset);

    G_debug(3, "  SQL: %s", buf);
    db_init_string(&stmt);
    db_set_string(&stmt, buf);

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error("Cannot select records from LRS table:\n%s", buf);

    table = db_get_cursor_table(&cursor);
    nrseg = db_get_num_rows(&cursor);
    G_debug(3, "  nrseg = %d", nrseg);

    if (nrseg <= 0)
	return 0;
    if (nrseg >= 3)
	return 2;

    rseg = (RSEGMENT *) G_malloc(nrseg * sizeof(RSEGMENT));

    i = 0;
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error("Cannot fetch line id from line table");

	if (!more)
	    break;

	column = db_get_table_column(table, 0);	/* first column */
	value = db_get_column_value(column);
	rseg[i].lid = db_get_value_int(value);

	column = db_get_table_column(table, 1);
	value = db_get_column_value(column);
	rseg[i].start_map = db_get_value_double(value);

	column = db_get_table_column(table, 2);
	value = db_get_column_value(column);
	rseg[i].end_map = db_get_value_double(value);

	column = db_get_table_column(table, 3);
	value = db_get_column_value(column);
	rseg[i].start_mp = db_get_value_double(value);

	column = db_get_table_column(table, 4);
	value = db_get_column_value(column);
	rseg[i].start_off = db_get_value_double(value);

	column = db_get_table_column(table, 5);
	value = db_get_column_value(column);
	rseg[i].end_mp = db_get_value_double(value);

	column = db_get_table_column(table, 6);
	value = db_get_column_value(column);
	rseg[i].end_off = db_get_value_double(value);

	i++;
    }
    db_close_cursor(&cursor);


    /* Two segments may be selected for one point if they share common MPs */
    /* If 2 segments were found, check if it is correct */
    if (nrseg == 2) {
	G_debug(4, "rseg[0].lid = %d rseg[1].lid = %d", rseg[0].lid,
		rseg[1].lid);
	G_debug(4, "rseg[0].start_map = %f rseg[0].end_map = %f",
		rseg[0].start_map, rseg[0].end_map);
	G_debug(4, "rseg[1].start_map = %f rseg[1].end_map = %f",
		rseg[1].start_map, rseg[1].end_map);

	/* Warning: start_map/end_map stored in the table may lose precision and will not be equal to
	 * map_offset */
	if (rseg[0].lid == rseg[1].lid &&
	    rseg[0].end_map == rseg[1].start_map) {
	    G_debug(4,
		    " point at the end of 1. end beginning of 2. segment -> OK");
	    *lid = rseg[0].lid;
	    *mpost = rseg[1].start_mp;
	    *offset = rseg[1].start_off;
	    return 1;
	}
	else if (rseg[0].lid == rseg[1].lid &&
		 rseg[1].end_map == rseg[0].start_map) {
	    G_debug(4,
		    " point at the end of 2. end beginning of 1. segment -> OK");
	    *lid = rseg[0].lid;
	    *mpost = rseg[0].start_mp;
	    *offset = rseg[0].start_off;
	    return 1;
	}
	else {
	    G_debug(4, " too many segments found in the table -> error ");
	    return 2;
	}
    }

    /* One segment was found -> calculate map_offset */
    *lid = rseg[0].lid;

    /* Real world length of segment in offset units */
    length =
	(rseg[0].end_map + rseg[0].end_off) - (rseg[0].start_map +
					       rseg[0].start_off);
    map_length = rseg[0].end_map - rseg[0].start_map;
    k = map_length / length;
    G_debug(4,
	    " rseg[0].end_mp=%f, rseg[0].end_off=%f, rseg[0].start_mp=%f, rseg[0].start_off=%f, multip=%f",
	    rseg[0].end_mp, rseg[0].end_off, rseg[0].start_mp,
	    rseg[0].start_off, multip);
    G_debug(4, " seg length=%f, seg map_length=%f, k=%f", length, map_length,
	    k);

    /* Milepost and offset */
    /* Offset from start milepost measured in map in map units */
    moff = map_offset - rseg[0].start_map;

    /* Offset from start milepost in real world in offset units */
    soff = moff / k;

    /* Offset in real world (from the beginning of 'feature') in offset units */
    roff = multip * rseg[0].start_mp + rseg[0].start_off + soff;

    G_debug(4, " moff = %f soff = %f roff = %f", moff, soff, roff);

    /* Translate offset to milepost + offset */
    mp = (int)roff / multip;
    off = roff - mp * multip;

    G_debug(4, " mp = %d off = %f", mp, off);

    /* It may happen, that 'end_off > multip', in that case mpost could be now > rseg[0].end_mp
     * which is incorrect */
    if (mp > rseg[0].end_mp) {
	mp = rseg[0].end_mp;
	off = roff - mp * multip;
    }
    G_debug(4, " mp = %d off = %f", mp, off);

    *mpost = mp;
    *offset = off;

    return 1;
}

/* For given lid (line id), mpost (milepost) + offset in real world find
 *  line_cat and map_offset along the line in the map from rstable 
 *  in opened database. 
 *  multip - multiplier specifies number of map units in mpost unit
 *  Returns: 0 - no offset found
 *           1 - one offset found
 *           3 - 2 or more records exactly at requested position with different line_cat/map_offset,
 *               the line_cat/map_offset is set to first found in direction up
 */
int LR_get_offset(dbDriver * driver, char *table_name,
		  char *lcat_col, char *lid_col,
		  char *start_map_col, char *end_map_col,
		  char *start_mp_col, char *start_off_col,
		  char *end_mp_col, char *end_off_col,
		  int lid, double mpost, double offset,
		  double multip, int *line_cat, double *map_offset)
{
    int ret;

    G_debug(3, "LR_get_offset() lid = %d, mpost = %f, offset = %f", lid,
	    mpost, offset);

    /* direction down -> first segment returned if multiple found */
    ret = LR_get_nearest_offset(driver, table_name, lcat_col, lid_col,
				start_map_col, end_map_col,
				start_mp_col, start_off_col, end_mp_col,
				end_off_col, lid, mpost, offset, multip, 1,
				line_cat, map_offset);

    if (ret == 2)
	return 0;

    return ret;
}

/* Find if requested lid/mpost/offset falls into given RSEGMENT
 *
 *  rseg - pointer to RSEGMENT
 *  multip - multiplier specifies number of map units in mpost unit
 *  mpost - requested MP
 *  offset - requested offset
 *  
 *  map_offset - output offset if the mpost/offset falls in rseg 
 *  
 *  Return: 0 - outside, map_offset not set
 *          1 - OK, mpost/offset falls in rseg, map_offset set
 */
int offset_in_rsegment(RSEGMENT * rseg, double multip, double mpost,
		       double offset, double *map_offset)
{
    int ret;
    double length, map_length, k;

    G_debug(3, "offset_in_rsegment: %f+%f rseg:  %f+%f - %f+%f", mpost,
	    offset, rseg->start_mp, rseg->start_off, rseg->end_mp,
	    rseg->end_off);

    *map_offset = 0.0;

    /* Check if >= start */
    ret = LR_cmp_mileposts(mpost, offset, rseg->start_mp, rseg->start_off);
    if (ret == -1) {
	G_debug(4, "  < start");
	return 0;
    }
    /* Check if <= end */
    ret = LR_cmp_mileposts(mpost, offset, rseg->end_mp, rseg->end_off);
    if (ret == 1) {
	G_debug(4, "  > end");
	return 0;
    }

    /* Within segment -> calculate map_offset */
    length = (multip * rseg->end_mp + rseg->end_off) -
	(multip * rseg->start_mp + rseg->start_off);
    map_length = rseg->end_map - rseg->start_map;
    k = map_length / length;

    G_debug(4, " seg length = %f seg map_length = %f k = %f", length,
	    map_length, k);

    /* length from last MP in real world */
    length = (multip * mpost + offset) -
	(multip * rseg->start_mp + rseg->start_off);
    G_debug(4, " length in real world from previous milepost = %f", length);

    *map_offset = rseg->start_map + k * length;

    G_debug(3, "map_offset = %f", *map_offset);

    return 1;
}

/* For given lid (line id), mpost (milepost) + offset in real world find
 *  nearest available (along the feature in specified direction)
 *  line_cat and map_offset along the line in the map from rstable 
 *  in opened database. 
 *  For example, if RS table contains only segment 2+000 - 3+000 and requested
 *  position is 1+500/direction up, the point returned will be 2+000
 *
 *  multip - multiplier specifies number of map units in mpost unit
 *  direction - direction in which search the nearest available position on linear feature
 *           0 - up (in the direction to the end)
 *           1 - down (in the direction to the beginning, to 0)
 *  Returns: 0 - offset not found
 *           1 - one offset found exactly at requested position
 *           2 - one nearest offset found in requested direction 
 *           3 - 2 or more records exactly at requested position with different line_cat/map_offset,
 *               the line_cat/map_offset is set to the last segment in the specified direction, 
 *               i.e. if direction is down, the first segment in direction up is used 
 *               (this is suitable for segment creation)
 * 
 */
int LR_get_nearest_offset(dbDriver * driver, char *table_name,
			  char *lcat_col, char *lid_col,
			  char *start_map_col, char *end_map_col,
			  char *start_mp_col, char *start_off_col,
			  char *end_mp_col, char *end_off_col,
			  int lid, double mpost, double offset,
			  double multip, int direction,
			  int *line_cat, double *map_offset)
{
    int i, more, ret;
    char buf[2000];
    dbString stmt;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;
    RSEGMENT *rseg;
    int nrseg;
    int current_line_cat;
    double current_map_offset;
    int seg_found;		/* number of segments found containing the offset */
    int differ;			/* more segments with different line_cat/offset */
    int nearest;		/* nearest found */

    G_debug(2, "LR_get_offset() lid = %d, mpost = %f, offset = %f", lid,
	    mpost, offset);

    *line_cat = 0;
    *map_offset = 0;

    /* 1. select all available records for requested lid
     * 2. order by start_mp, start_off (dbf driver does not support order by more columns)
     * 3. search in requested direction until either 
     *         a. requested position is reached
     *         b. next nearest segment is reached
     */

    sprintf(buf, "select %s, %s, %s, %s, %s, %s, %s from %s where %s = %d",
	    lcat_col, start_map_col, end_map_col,
	    start_mp_col, start_off_col, end_mp_col, end_off_col,
	    table_name, lid_col, lid);

    G_debug(3, "  SQL: %s", buf);
    db_init_string(&stmt);
    db_append_string(&stmt, buf);

    if (db_open_select_cursor(driver, &stmt, &cursor, DB_SEQUENTIAL) != DB_OK)
	G_fatal_error("Cannot select records from LRS table:\n%s", buf);

    table = db_get_cursor_table(&cursor);
    nrseg = db_get_num_rows(&cursor);
    G_debug(3, "nrseg = %d", nrseg);

    rseg = (RSEGMENT *) G_malloc(nrseg * sizeof(RSEGMENT));

    i = 0;
    while (1) {
	if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
	    G_fatal_error("Cannot fetch line id from line table");

	if (!more)
	    break;

	column = db_get_table_column(table, 0);	/* first column */
	value = db_get_column_value(column);
	rseg[i].lcat = db_get_value_int(value);

	column = db_get_table_column(table, 1);
	value = db_get_column_value(column);
	rseg[i].start_map = db_get_value_double(value);

	column = db_get_table_column(table, 2);
	value = db_get_column_value(column);
	rseg[i].end_map = db_get_value_double(value);

	column = db_get_table_column(table, 3);
	value = db_get_column_value(column);
	rseg[i].start_mp = db_get_value_double(value);

	column = db_get_table_column(table, 4);
	value = db_get_column_value(column);
	rseg[i].start_off = db_get_value_double(value);

	column = db_get_table_column(table, 5);
	value = db_get_column_value(column);
	rseg[i].end_mp = db_get_value_double(value);

	column = db_get_table_column(table, 6);
	value = db_get_column_value(column);
	rseg[i].end_off = db_get_value_double(value);

	G_debug(3, "  start_mp = %f start_off = %f", rseg[i].start_mp,
		rseg[i].start_off);

	i++;
    }
    db_close_cursor(&cursor);

    qsort((void *)rseg, (unsigned int)nrseg, sizeof(RSEGMENT), cmp_rsegment);

    /* Select the right segment */
    /* Two segments may be selected for one point if they share common MPs */

    seg_found = 0;
    differ = 0;
    nearest = 0;
    if (direction == 0) {	/* direction up */
	for (i = 0; i < nrseg; i++) {
	    double off;

	    G_debug(3, "up: %d: %f - %f %f+%f %f+%f", rseg[i].lcat,
		    rseg[i].start_map, rseg[i].end_map,
		    rseg[i].start_mp, rseg[i].start_off,
		    rseg[i].end_mp, rseg[i].end_off);

	    ret = offset_in_rsegment(&(rseg[i]), multip, mpost, offset, &off);

	    if (ret == 0) {	/* outside segment */
		if (seg_found == 0) {
		    /* It can be that we passed already requested position, in that case 
		     * return the first found */
		    if (-1 ==
			LR_cmp_mileposts(mpost, offset, rseg[i].start_mp,
					 rseg[i].start_off)) {
			current_line_cat = rseg[i].lcat;
			current_map_offset = off;
			G_debug(3,
				"requested position passed -> use nearest: line_cat = %d, map_offset = %f ",
				current_line_cat, current_map_offset);
			seg_found = 1;
			nearest = 1;
			break;
		    }
		    else {
			continue;	/* not yet reached */
		    }
		}
		else {
		    break;	/* segment found and  passed */
		}
	    }

	    if (seg_found) {	/* next found */
		/* Check if it is the same offset */
		/* Because offsets calculated from different segments may differ because of representation
		 * error we have to use LRS_THRESH */
		/* TODO: avoid LRS_THRESH ? */
		if (rseg[i].lcat != current_line_cat ||
		    fabs(off - current_map_offset) > LRS_THRESH) {
		    G_debug(3,
			    "%d != %d ||  fab(soff-current_map_offset) = %e > LRS_THRESH",
			    rseg[i].lcat, current_line_cat,
			    fabs(off - current_map_offset));
		    differ = 1;
		}
	    }
	    current_line_cat = rseg[i].lcat;
	    current_map_offset = off;

	    seg_found++;
	}
    }
    else {			/* direction down */
	for (i = nrseg - 1; i >= 0; i--) {
	    double off;

	    G_debug(3, "down: %d: %f - %f %f+%f %f+%f", rseg[i].lcat,
		    rseg[i].start_map, rseg[i].end_map,
		    rseg[i].start_mp, rseg[i].start_off,
		    rseg[i].end_mp, rseg[i].end_off);

	    ret = offset_in_rsegment(&(rseg[i]), multip, mpost, offset, &off);

	    if (ret == 0) {	/* outside segment */
		if (seg_found == 0) {
		    /* It can be that we passed already requested position, in that case 
		     * return the first found */
		    if (1 ==
			LR_cmp_mileposts(mpost, offset, rseg[i].end_mp,
					 rseg[i].end_off)) {
			current_line_cat = rseg[i].lcat;
			current_map_offset = off;
			G_debug(3,
				"requested position passed -> use nearest: line_cat = %d, map_offset = %f ",
				current_line_cat, current_map_offset);
			seg_found = 1;
			nearest = 1;
			break;
		    }
		    else {
			continue;	/* not yet reached */
		    }
		}
		else {
		    break;	/* segment found and  passed */
		}
	    }

	    if (seg_found) {	/* next found */
		/* Check if it is the same offset */
		/* Because offsets calculated from different segments may differ because of representation
		 * error we have to use LRS_THRESH */
		/* TODO: avoid LRS_THRESH ? */
		if (rseg[i].lcat != current_line_cat ||
		    fabs(off - current_map_offset) > LRS_THRESH) {
		    G_debug(3,
			    "%d != %d ||  fab(soff-current_map_offset) = %e > LRS_THRESH",
			    rseg[i].lcat, current_line_cat,
			    fabs(off - current_map_offset));
		    differ = 1;
		}
	    }
	    current_line_cat = rseg[i].lcat;
	    current_map_offset = off;

	    seg_found++;
	}
    }

    free(rseg);

    /* Was any segment found ? */
    if (seg_found == 0) {	/* no */
	G_debug(2, " no segment found in the reference table");
	return 0;
    }

    G_debug(2, " lcat = %d map_offset = %f", current_line_cat,
	    current_map_offset);

    *line_cat = current_line_cat;
    *map_offset = current_map_offset;

    /* More segments */
    if (seg_found > 1) {
	if (differ) {
	    G_debug(2,
		    " point within more segments with different line_cat/map_offset");
	    return 3;
	}
	else {
	    G_debug(2,
		    " point within more segments with the same line_cat/map_offset (OK)");
	}
    }

    if (nearest)
	return 2;

    return 1;
}

/* Compare 2 mileposts 
 *  Returns: -1 if 1. < 2.
 *            0    1. = 2.
 *            1    1.  > 2.
 */
int LR_cmp_mileposts(double mp1, double off1, double mp2, double off2)
{
    G_debug(3, "LR_cmp_mileposts(): %f+%f x %f+%f", mp1, off1, mp2, off2);

    /* Note: this should be safe, because mp may be only integers */

    /* compare mp */
    if (mp1 < mp2) {
	G_debug(4, "1. < 2.");
	return -1;
    }
    if (mp1 > mp2) {
	G_debug(4, "1. > 2.");
	return 1;
    }
    /* the same mp -> compare off */
    if (off1 < off2) {
	G_debug(4, "1. < 2.");
	return -1;
    }
    if (off1 > off2) {
	G_debug(4, "1. > 2.");
	return 1;
    }
    G_debug(4, "1. = 2.");
    return 0;
}
