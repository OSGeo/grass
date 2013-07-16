#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int thin_seg(int stream_id)
{
    int thinned = 0;
    int r, c, r_nbr, c_nbr, last_r, last_c;
    CELL curr_stream, no_stream = 0;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    ASP_FLAG af;

    r = stream_node[stream_id].r;
    c = stream_node[stream_id].c;

    cseg_get(&stream, &curr_stream, r, c);

    seg_get(&aspflag, (char *)&af, r, c);
    if (af.asp > 0) {
	/* get downstream point */
	last_r = r + asp_r[(int)af.asp];
	last_c = c + asp_c[(int)af.asp];
	cseg_get(&stream, &curr_stream, last_r, last_c);

	if (curr_stream != stream_id)
	    return thinned;

	/* get next downstream point */
	seg_get(&aspflag, (char *)&af, last_r, last_c);
	while (af.asp > 0) {
	    r_nbr = last_r + asp_r[(int)af.asp];
	    c_nbr = last_c + asp_c[(int)af.asp];

	    if (r_nbr == last_r && c_nbr == last_c)
		return thinned;
	    if (r_nbr < 0 || r_nbr >= nrows || c_nbr < 0 || c_nbr >= ncols)
		return thinned;
	    cseg_get(&stream, &curr_stream, r_nbr, c_nbr);
	    if (curr_stream != stream_id)
		return thinned;
	    if (abs(r_nbr - r) < 2 && abs(c_nbr - c) < 2) {
		/* eliminate last point */
		cseg_put(&stream, &no_stream, last_r, last_c);
		FLAG_UNSET(af.flag, STREAMFLAG);
		seg_put(&aspflag, (char *)&af, last_r, last_c);
		/* update start point */
		seg_get(&aspflag, (char *)&af, r, c);
		af.asp = drain[r - r_nbr + 1][c - c_nbr + 1];
		seg_put(&aspflag, (char *)&af, r, c);

		thinned = 1;
	    }
	    else {
		/* nothing to eliminate, continue from last point */
		r = last_r;
		c = last_c;
	    }
	    last_r = r_nbr;
	    last_c = c_nbr;
	    seg_get(&aspflag, (char *)&af, last_r, last_c);
	}
    }

    return thinned;
}

int thin_streams(void)
{
    int i, j, r, c, done;
    CELL stream_id;
    int next_node;
    struct sstack
    {
	int stream_id;
	int next_trib;
    } *nodestack;
    int top = 0, stack_step = 1000;
    int n_trib_total;
    int n_thinned = 0;

    G_message(_("Thinning stream segments..."));

    nodestack = (struct sstack *)G_malloc(stack_step * sizeof(struct sstack));

    for (i = 0; i < n_outlets; i++) {
	G_percent(i, n_outlets, 2);
	r = outlets[i].r;
	c = outlets[i].c;
	cseg_get(&stream, &stream_id, r, c);

	if (stream_id == 0)
	    continue;

	/* add root node to stack */
	G_debug(2, "add root node");
	top = 0;
	nodestack[top].stream_id = stream_id;
	nodestack[top].next_trib = 0;

	/* depth first post order traversal */
	G_debug(2, "traverse");
	while (top >= 0) {

	    done = 1;
	    stream_id = nodestack[top].stream_id;
	    G_debug(3, "stream_id %d, top %d", stream_id, top);
	    if (nodestack[top].next_trib < stream_node[stream_id].n_trib) {
		/* add to stack */
		G_debug(3, "get next node");
		next_node =
		    stream_node[stream_id].trib[nodestack[top].next_trib];
		G_debug(3, "add to stack: next %d, trib %d, n trib %d",
			next_node, nodestack[top].next_trib,
			stream_node[stream_id].n_trib);
		nodestack[top].next_trib++;
		top++;
		if (top >= stack_step) {
		    /* need more space */
		    stack_step += 1000;
		    nodestack =
			(struct sstack *)G_realloc(nodestack,
						   stack_step *
						   sizeof(struct sstack));
		}

		nodestack[top].next_trib = 0;
		nodestack[top].stream_id = next_node;
		done = 0;
		G_debug(3, "go further down");
	    }
	    if (done) {
		/* thin stream segment */
		G_debug(3, "thin stream segment %d", stream_id);

		if (thin_seg(stream_id) == 0)
		    G_debug(3, "segment %d not thinned", stream_id);
		else {
		    G_debug(3, "segment %d thinned", stream_id);
		    n_thinned++;
		}

		top--;
		/* count tributaries */
		if (top >= 0) {
		    n_trib_total = 0;
		    stream_id = nodestack[top].stream_id;
		    for (j = 0; j < stream_node[stream_id].n_trib; j++) {
			/* intermediate */
			if (stream_node[stream_node[stream_id].trib[j]].
			    n_trib > 0)
			    n_trib_total +=
				stream_node[stream_node[stream_id].trib[j]].
				n_trib_total;
			/* start */
			else
			    n_trib_total++;
		    }
		    stream_node[stream_id].n_trib_total = n_trib_total;
		}
	    }
	}
    }
    G_percent(n_outlets, n_outlets, 1);	/* finish it */

    G_free(nodestack);
    
    G_verbose_message(_("%d of %lld stream segments were thinned"), n_thinned, n_stream_nodes);

    return 1;
}
