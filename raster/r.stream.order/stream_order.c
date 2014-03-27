/* 
   All algorithms used in analysis ar not recursive. For Strahler order and Shreve magnitude starts from initial channel and  proceed downstream. Algortitms try to assgin order for branch and if it is imposible start from next initial channel, till all branches are ordered.
   For Hortor and Hack ordering it proceed upstream and uses stack data structure to determine unordered branch. 
   Algorithm of Hack main stram according idea of Markus Metz.
 */

#include "local_proto.h"
int strahler(int *strahler)
{

    int i, j, done = 1;
    int cur_stream, next_stream;
    int max_strahler = 0, max_strahler_num;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_message(_("Calculating Strahler's stream order..."));

    for (j = 0; j < init_num; ++j) {	/* main loop on inits */

	cur_stream = SA[init_streams[j]].stream;
	do {			/* we must go at least once, if stream is of first order and is outlet */
	    max_strahler_num = 1;
	    max_strahler = 0;
	    next_stream = SA[cur_stream].next_stream;

	    if (SA[cur_stream].trib_num == 0) {	/* assign 1 for spring stream */
		strahler[cur_stream] = 1;
		cur_stream = next_stream;
		done = 1;
	    }
	    else {
		done = 1;

		for (i = 0; i < SA[cur_stream].trib_num; ++i) {	/* loop for determining strahler */
		    if (strahler[SA[cur_stream].trib[i]] < 0) {
			done = 0;
			break;	/* strahler is not determined, break for loop */
		    }
		    else if (strahler[SA[cur_stream].trib[i]] > max_strahler) {
			max_strahler = strahler[SA[cur_stream].trib[i]];
			max_strahler_num = 1;
		    }
		    else if (strahler[SA[cur_stream].trib[i]] == max_strahler) {
			++max_strahler_num;
		    }
		}		/* end determining strahler */

		if (done == 1) {
		    strahler[cur_stream] = (max_strahler_num > 1) ?
			++max_strahler : max_strahler;
		    cur_stream = next_stream;	/* if next_stream<0 we in outlet stream */
		}

	    }
	} while (done && next_stream > 0);
    }				/* end for of main loop */
    return 0;
}				/* end strahler */

int shreve(int *shreve)
{

    int i, j, done = 1;
    int cur_stream, next_stream;
    int max_shreve = 0;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_message(_("Calculating Shreve's stream magnitude, "
                "Scheidegger's consistent integer and Drwal's streams hierarchy (old style)..."));

    for (j = 0; j < init_num; ++j) {	/* main loop on inits */

	cur_stream = SA[init_streams[j]].stream;
	do {			/* we must go at least once, if stream is of first order and is outlet */

	    max_shreve = 0;
	    next_stream = SA[cur_stream].next_stream;

	    if (SA[cur_stream].trib_num == 0) {	/* assign 1 for spring stream */

		shreve[cur_stream] = 1;
		cur_stream = next_stream;
		done = 1;

	    }
	    else {
		done = 1;

		for (i = 0; i < SA[cur_stream].trib_num; ++i) {	/* loop for determining shreve */
		    if (shreve[SA[cur_stream].trib[i]] < 0) {
			done = 0;
			break;	/* shreeve is not determined, break for loop */
		    }
		    else {
			max_shreve += shreve[SA[cur_stream].trib[i]];
		    }
		}		/* end determining shreve */

		if (done == 1) {
		    shreve[cur_stream] = max_shreve;
		    cur_stream = next_stream;	/* if next_stream<0 we in outlet stream */
		}
	    }

	} while (done && next_stream > 0);
    }				/* end main loop */
    return 0;
}				/* end shreeve */

int horton(const int *strahler, int *horton, int number_of_streams)
{

    int *stack;
    int stack_max = number_of_streams;
    int top, i, j;
    int cur_stream, cur_horton;
    int max_strahler;
    double max_accum, accum;
    int up_stream = 0;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_message(_("Calculating Hortons's stream order..."));
    stack = (int *)G_malloc(stack_max * sizeof(int));

    for (j = 0; j < outlet_num; ++j) {
	cur_stream = SA[outlet_streams[j]].stream;	/* outlet: init */
	cur_horton = strahler[cur_stream];
	stack[0] = 0;
	stack[1] = cur_stream;
	top = 1;

	do {			/* on every stream */
	    max_strahler = 0;
	    max_accum = 0;

	    if (SA[cur_stream].trib_num == 0) {	/* spring: go back on stack */

		horton[cur_stream] = cur_horton;
		cur_stream = stack[--top];

	    }
	    else if (SA[cur_stream].trib_num > 1) {	/* node */

		up_stream = 0;	/* calculating up_stream */
		for (i = 0; i < SA[cur_stream].trib_num; ++i) {
		    if (horton[SA[cur_stream].trib[i]] < 0) {

			if (strahler[SA[cur_stream].trib[i]] > max_strahler) {
			    max_strahler = strahler[SA[cur_stream].trib[i]];
			    max_accum =
				(use_accum) ? SA[SA[cur_stream].trib[i]].
				accum : SA[SA[cur_stream].trib[i]].
				accum_length;
			    up_stream = SA[cur_stream].trib[i];

			}
			else if (strahler[SA[cur_stream].trib[i]] ==
				 max_strahler) {

			    accum =
				(use_accum) ? SA[SA[cur_stream].trib[i]].
				accum : SA[SA[cur_stream].trib[i]].
				accum_length;

			    if (accum > max_accum) {
				max_accum =
				    (use_accum) ? SA[SA[cur_stream].trib[i]].
				    accum : SA[SA[cur_stream].trib[i]].
				    accum_length;

				up_stream = SA[cur_stream].trib[i];
			    }
			}
		    }
		}		/* end determining up_stream */

		if (up_stream) {	/* at least one branch is not assigned */
		    if (horton[cur_stream] < 0) {
			horton[cur_stream] = cur_horton;
		    }
		    else {
			cur_horton = strahler[up_stream];
		    }
		    cur_stream = up_stream;
		    stack[++top] = cur_stream;

		}
		else {		/* all asigned, go downstream */
		    cur_stream = stack[--top];

		}		/* end up_stream */
	    }			/* end spring/node */
	} while (cur_stream);
    }				/* end for outlets */
    G_free(stack);
    return 0;
}

int hack(int *hack, int *topo_dim, int number_of_streams)
{				/* also calculate topological dimension */

    int *stack;
    int top, i, j;
    int cur_stream, cur_hack;
    double accum, max_accum;
    int up_stream = 0;
    int stack_max = number_of_streams;
    double cur_distance = 0;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_message(_("Calculating Hack's main streams and topological dimension..."));
    stack = (int *)G_malloc(stack_max * sizeof(int));

    for (j = 0; j < outlet_num; ++j) {

	cur_stream = SA[outlet_streams[j]].stream;	/* outlet: init */
	cur_hack = 1;
	stack[0] = 0;
	stack[1] = cur_stream;
	top = 1;

	topo_dim[cur_stream] = top;
	cur_distance = SA[cur_stream].distance = SA[cur_stream].length;
	do {
	    max_accum = 0;

	    if (SA[cur_stream].trib_num == 0) {	/* spring: go back on stack */

		hack[cur_stream] = cur_hack;
		cur_stream = stack[--top];

	    }
	    else if (SA[cur_stream].trib_num > 1) {	/* node */
		up_stream = 0;	/* calculating up_stream */

		for (i = 0; i < SA[cur_stream].trib_num; ++i) {	/* determining upstream */
		    if (hack[SA[cur_stream].trib[i]] < 0) {

			accum =
			    (use_accum) ? SA[SA[cur_stream].trib[i]].
			    accum : SA[SA[cur_stream].trib[i]].accum_length;

			if (accum > max_accum) {
			    max_accum =
				(use_accum) ? SA[SA[cur_stream].trib[i]].
				accum : SA[SA[cur_stream].trib[i]].
				accum_length;
			    up_stream = SA[cur_stream].trib[i];
			}
		    }
		}		/* end determining up_stream */

		if (up_stream) {	/* at least one branch is not assigned */

		    if (hack[cur_stream] < 0) {
			hack[cur_stream] = cur_hack;
		    }
		    else {
			cur_hack = hack[cur_stream];
			++cur_hack;
		    }

		    cur_distance = SA[cur_stream].distance;
		    cur_stream = up_stream;
		    stack[++top] = cur_stream;
		    SA[cur_stream].distance =
			cur_distance + SA[cur_stream].length;
		    topo_dim[cur_stream] = top;
		}
		else {		/* all asigned, go downstream */
		    cur_stream = stack[--top];

		}		/* end up_stream */
	    }			/* end spring/node */
	} while (cur_stream);
    }				/* end for outlets */
    G_free(stack);
    return 0;
}
