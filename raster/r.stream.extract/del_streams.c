#include <stdlib.h>
#include <math.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* get stream segment length */
int seg_length(CELL stream_id, CELL *next_stream_id)
{
    int r, c, r_nbr, c_nbr;
    int slength = 1;
    CELL curr_stream;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    ASP_FLAG af;

    r = stream_node[stream_id].r;
    c = stream_node[stream_id].c;
    if (next_stream_id)
	*next_stream_id = stream_id;

    /* get next downstream point */
    seg_get(&aspflag, (char *)&af, r, c);
    while (af.asp > 0) {
	r_nbr = r + asp_r[(int)af.asp];
	c_nbr = c + asp_c[(int)af.asp];

	/* user-defined depression */
	if (r_nbr == r && c_nbr == c)
	    break;
	/* outside region */
	if (r_nbr < 0 || r_nbr >= nrows || c_nbr < 0 || c_nbr >= ncols)
	    break;
	/* next stream */
	cseg_get(&stream, &curr_stream, r_nbr, c_nbr);
	if (next_stream_id)
	    *next_stream_id = curr_stream;
	if (curr_stream != stream_id)
	    break;
	slength++;
	r = r_nbr;
	c = c_nbr;
	seg_get(&aspflag, (char *)&af, r, c);
    }

    return slength;
}

/* change downstream id: update or remove */
int update_stream_id(CELL stream_id, CELL new_stream_id)
{
    int i, r, c, r_nbr, c_nbr;
    CELL new_stream = new_stream_id;
    CELL curr_stream;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    ASP_FLAG af;

    r = stream_node[stream_id].r;
    c = stream_node[stream_id].c;
    cseg_get(&stream, &curr_stream, r, c);
    if (curr_stream != stream_id)
	G_fatal_error("update downstream id: curr_stream != stream_id");
    cseg_put(&stream, &new_stream, r, c);
    curr_stream = stream_id;

    /* get next downstream point */
    seg_get(&aspflag, (char *)&af, r, c);
    while (af.asp > 0) {
	r_nbr = r + asp_r[(int)af.asp];
	c_nbr = c + asp_c[(int)af.asp];

	/* user-defined depression */
	if (r_nbr == r && c_nbr == c)
	    break;
	/* outside region */
	if (r_nbr < 0 || r_nbr >= nrows || c_nbr < 0 || c_nbr >= ncols)
	    break;
	/* next stream */
	cseg_get(&stream, &curr_stream, r_nbr, c_nbr);
	if (curr_stream != stream_id)
	    break;
	r = r_nbr;
	c = c_nbr;
	cseg_put(&stream, &new_stream, r, c);
	seg_get(&aspflag, (char *)&af, r, c);
    }
    
    if (curr_stream <= 0)
	return curr_stream;

    /* update tributaries */
    if (curr_stream != stream_id) {
	int last_i = -1;
	
	for (i = 0; i < stream_node[curr_stream].n_trib; i++) {
	    if (stream_node[curr_stream].trib[i] == stream_id) {
		if (new_stream_id) {
		    stream_node[curr_stream].trib[i] = new_stream_id;
		}
		else {
		    stream_node[curr_stream].n_trib--;
		    stream_node[curr_stream].trib[i] = stream_node[curr_stream].trib[stream_node[curr_stream].n_trib];
		    stream_node[curr_stream].trib[stream_node[curr_stream].n_trib] = 0;
		}
		last_i = i;
		break;
	    }
	}
	for (i = 0; i < stream_node[curr_stream].n_trib; i++) {
	    if (stream_node[curr_stream].trib[i] == stream_id) {
		G_warning("last_i %d, i %d", last_i, i);
		G_warning("failed updating stream %d at node %d", stream_id, curr_stream);
	    }
	}
    }

    return curr_stream;
}

/* delete stream segments shorter than threshold */
int del_streams(int min_length)
{
    int i;
    int n_deleted = 0;
    CELL curr_stream, stream_id;
    int other_trib, tmp_trib;
    int slength;

    G_message(_("Deleting stream segments shorter than %d cells..."), min_length);

    /* TODO: proceed from stream heads to outlets
     *       -> use depth first post order traversal */

    /* go through all nodes */
    for (i = 1; i <= n_stream_nodes; i++) {
	G_percent(i, n_stream_nodes, 2);

	/* not a stream head */
	if (stream_node[i].n_trib > 0)
	    continue;

	/* already deleted */
	cseg_get(&stream, &curr_stream, stream_node[i].r, stream_node[i].c);
	if (curr_stream == 0)
	    continue;

	/* get length counted as n cells */
	if ((slength = seg_length(i, &curr_stream)) >= min_length)
	    continue;

	stream_id = i;
	
	/* check n sibling tributaries */
	if (curr_stream != stream_id) {
	    /* only one sibling tributary */
	    if (stream_node[curr_stream].n_trib == 2) {
		if (stream_node[curr_stream].trib[0] != stream_id)
		    other_trib = stream_node[curr_stream].trib[0];
		else
		    other_trib = stream_node[curr_stream].trib[1];

		/* other trib is also stream head */
		if (stream_node[other_trib].n_trib == 0) {
		    /* use shorter one */
		    if (seg_length(other_trib, NULL) < slength) {
			tmp_trib = stream_id;
			stream_id = other_trib;
			other_trib = tmp_trib;
		    }
		}
		update_stream_id(stream_id, 0);
		n_deleted++;
		
		/* update downstream IDs */
		update_stream_id(curr_stream, other_trib);
	    }
	    /* more than one other tributary */
	    else {
		update_stream_id(stream_id, 0);
		n_deleted++;
	    }
	}
	/* stream head is also outlet */
	else {
	    update_stream_id(stream_id, 0);
	    n_deleted++;
	}
    }

    G_verbose_message(_("%d stream segments deleted"), n_deleted);

    return n_deleted;
}
