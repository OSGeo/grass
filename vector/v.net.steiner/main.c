
/****************************************************************
 * 
 *  MODULE:       v.net.steiner
 *  
 *  AUTHOR(S):    Radim Blazek
 *                
 *  PURPOSE:      Find Steiner tree for network
 *                
 *  COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * 
 *                This program is free software under the 
 *                GNU General Public License (>=v2). 
 *                Read the file COPYING that comes with GRASS
 *                for details.
 * 
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* costs between 2 terminals */
typedef struct
{
    int term1, term2;
    double cost;
} COST;

int nterms;			/* number of terminals */
int nnodes;			/* number of nodes */
int *terms;			/* array of 1. terminals; 2. accepted steiner points; 3. tested steiner point */
COST *term_costs;		/* costs between terminals */
COST *sp_costs;			/* costs between StP and terminals */
int *comps;			/* numbers of component, the terminal was assigned to */

/* pointer to array of pointers to costs to other nodes, matrix: from-to, from < to:
 *  1-2 1-3 1-4 ... 1-nnodes
 *  2-3 2-4 ... 2-nnodes
 *  3-4 ... 3-nnodes
 *  ...
 *  (nnodes - 1)-nnodes
 *  !!! init costs for from or to node, before this cost is read by init_node_costs();
 */
double **nodes_costs;

int cmp(const void *, const void *);

/* Init all costs to/from given node */
int init_node_costs(struct Map_info *Map, int from)
{
    int to, ret, row, col;
    double cost;

    G_message(_("Init costs from node %d"), from);

    for (to = 1; to <= nnodes; to++) {
	if (from == to)
	    continue;
	ret = Vect_net_shortest_path(Map, from, to, NULL, &cost);
	if (ret == -1) {
	    /* G_warning ( "Destination node %d is unreachable from node %d\n", to, from); */
	    cost = -2;
	}

	if (from < to) {
	    row = from - 1;
	    col = to - from - 1;
	}
	else {
	    row = to - 1;
	    col = from - to - 1;
	}

	G_debug(3, "init costs %d - > %d = %f\n", from, to, cost);
	nodes_costs[row][col] = cost;
    }

    return 1;
}

/* Get cost from node to node.
 *  From or to costs must be init before!!!
 *  Returns: 1 - ok, 0 - not reachable
 */
int get_node_costs(int from, int to, double *cost)
{
    int row, col, tmp;

    if (from == to) {
	*cost = 0;
	return 1;
    }
    if (from > to) {
	tmp = from;
	from = to;
	to = tmp;
    }
    row = from - 1;
    col = to - from - 1;

    if (nodes_costs[row][col] == -2) {
	*cost = PORT_DOUBLE_MAX;
	return 0;
    }

    *cost = nodes_costs[row][col];
    return 1;
}

/* Calculate costs for MST on given set of terminals.
 *  If AList / NList is not NULL, list of arcs / nodes in MST is created
 *  Note: qsort() from more (say >30) terminals, takes long time (most of mst()).
 *        To improve the speed, there are used two sorted queues of costs:
 *          1. for all combinations of in trms
 *          2. from 'sp' to all other terminals
 *        Because 1. is sorted only if new sp as added to list of terminals,
 *        and 2. is much shorter than 1., a lot of time is saved.
 */
int mst(struct Map_info *Map, int *trms, int ntrms,	/* array of terminal, number of terminals */
	double *cst, double max_cst,	/* cost, maximum cost */
	struct ilist *AList, struct ilist *NList,	/* list of arcs/nodes in ST */
	int sp,			/* Steiner point (node) to be tested with terminals, (0 = ignore) */
	int rebuild)
{				/* rebuild the sorted list of costs for terminals */
    int i, j, node1, node2, com1, com2, t1, t2, line;
    static int k;
    int tcpos, scpos;		/* current position in the term_costs / sp_costs */
    double tcst;
    struct ilist *List;
    int nsteps, quse;
    int nall;			/* number of terminals + sp ( if used ) */

    if (AList != NULL) {
	Vect_reset_list(AList);
    }
    List = Vect_new_list();

    /* Create sorted array for all combinations of terms */
    if (rebuild) {
	k = 0;
	for (i = 0; i < ntrms; i++) {
	    for (j = i + 1; j < ntrms; j++) {
		term_costs[k].term1 = i;
		term_costs[k].term2 = j;
		get_node_costs(trms[i], trms[j], &tcst);
		term_costs[k].cost = tcst;
		k++;
	    }
	}

	qsort((void *)term_costs, k, sizeof(COST), cmp);	/* this takes most of a time in mst() */
	for (i = 0; i < k; i++) {
	    G_debug(3, "  %d - %d cost = %f\n", term_costs[i].term1,
		    term_costs[i].term2, term_costs[i].cost);
	}
    }

    /* Create sorted array for all combinations of sp -> terms */
    if (sp > 0) {
	for (i = 0; i < ntrms; i++) {
	    sp_costs[i].term1 = -1;	/* not needed */
	    sp_costs[i].term2 = i;
	    get_node_costs(sp, trms[i], &tcst);
	    sp_costs[i].cost = tcst;
	}
	qsort((void *)sp_costs, ntrms, sizeof(COST), cmp);
	for (i = 0; i < ntrms; i++) {
	    G_debug(3, "  %d - %d cost = %f\n", sp_costs[i].term1,
		    sp_costs[i].term2, sp_costs[i].cost);
	}
    }

    tcst = 0;

    /* MST has number_of_terminals-1 arcs */
    if (sp > 0) {
	nall = ntrms + 1;
	nsteps = ntrms;		/* i.e. + one StP */
    }
    else {
	nall = ntrms;
	nsteps = ntrms - 1;
    }
    G_debug(1, "nall = %d\n", nall);
    for (i = 0; i < nall; i++)
	comps[i] = 0;

    tcpos = 0;
    scpos = 0;
    G_debug(2, "nsteps = %d\n", nsteps);
    for (i = 0; i < nsteps; i++) {
	G_debug(2, "step = %d\n", i);
	/* Take the best (lowest costs, no cycle) from both queues */
	/* For both queues go to next lowest costs without cycle */
	/* treminal costs */
	for (j = tcpos; j < k; j++) {
	    t1 = term_costs[j].term1;
	    t2 = term_costs[j].term2;
	    com1 = comps[t1];
	    com2 = comps[t2];
	    if (com1 != com2 || com1 == 0) {	/* will not create cycle -> candidate */
		tcpos = j;
		break;
	    }
	}
	if (j == k) {		/* arc without cycle not found */
	    tcpos = -1;
	}

	/* StP costs */
	if (sp > 0) {
	    for (j = scpos; j < ntrms; j++) {
		t1 = ntrms;	/* StP is on first fre position */
		t2 = sp_costs[j].term2;
		com1 = comps[t1];
		com2 = comps[t2];
		G_debug(3, "scpos: j = %d comps(%d) = %d coms(%d) = %d\n", j,
			t1, com1, t2, com2);
		if (com1 != com2 || com1 == 0) {	/* will not create cycle -> candidate */
		    scpos = j;
		    G_debug(3, " ok -> scpos = %d\n", scpos);
		    break;
		}
	    }
	    if (j == ntrms) {	/* arc without cycle not found */
		scpos = -1;
	    }
	}
	else {
	    scpos = -1;
	}
	G_debug(3, "tcpos = %d, scpos = %d\n", tcpos, scpos);
	G_debug(3, "tcost = %f, scost = %f\n", term_costs[tcpos].cost,
		sp_costs[scpos].cost);

	/* Now we have positions set on lowest costs in each queue or -1 if no more/not used */
	if (tcpos >= 0 && scpos >= 0) {
	    if (term_costs[tcpos].cost < sp_costs[scpos].cost)
		quse = 1;	/* use terms queue */
	    else
		quse = 2;	/* use sp queue */
	}
	else if (tcpos >= 0) {
	    quse = 1;		/* use terms queue */
	}
	else {
	    quse = 2;		/* use sp queue */
	}


	/* Now we know from which queue take next arc -> add arc to components */
	if (quse == 1) {
	    t1 = term_costs[tcpos].term1;
	    t2 = term_costs[tcpos].term2;
	    tcst += term_costs[tcpos].cost;
	    tcpos++;
	}
	else {
	    t1 = ntrms;
	    t2 = sp_costs[scpos].term2;
	    tcst += sp_costs[scpos].cost;
	    scpos++;
	}
	G_debug(3, "quse = %d t1 = %d t2 = %d\n", quse, t1, t2);
	G_debug(3, "tcst = %f (max = %f)\n", tcst, max_cst);

	com1 = comps[t1];
	com2 = comps[t2];
	comps[t1] = i + 1;
	comps[t2] = i + 1;
	G_debug(3, "comps(%d) = %d coms(%d) = %d\n", t1, i + 1, t2, i + 1);

	/* reset connected branches */
	for (j = 0; j < nall; j++) {
	    if (comps[j] == com1 && com1 != 0)
		comps[j] = i + 1;

	    if (comps[j] == com2 && com2 != 0)
		comps[j] = i + 1;
	}
	if (tcst > max_cst) {
	    G_debug(3, "cost > max -> return\n");
	    *cst = PORT_DOUBLE_MAX;
	    return 1;
	}

	/* add to list of arcs */
	if (AList != NULL) {
	    node1 = trms[t1];
	    node2 = trms[t2];
	    Vect_net_shortest_path(Map, node1, node2, List, NULL);
	    for (j = 0; j < List->n_values; j++) {
		Vect_list_append(AList, abs(List->value[j]));
	    }
	}
    }

    /* create list of nodes */
    if (NList != NULL) {
	Vect_reset_list(NList);
	for (i = 0; i < AList->n_values; i++) {
	    line = AList->value[i];
	    Vect_get_line_nodes(Map, line, &node1, &node2);
	    Vect_list_append(NList, node1);
	    Vect_list_append(NList, node2);
	}
    }

    *cst = tcst;

    Vect_destroy_list(List);

    return 1;
}

int main(int argc, char **argv)
{
    int i, j, k, ret;
    int nlines, type, ltype, afield, tfield, geo, cat;
    int sp, nsp, nspused, node, line;
    struct Option *map, *output, *afield_opt, *tfield_opt, *afcol, *type_opt,
	*term_opt, *nsp_opt;
    struct Flag *geo_f;
    struct GModule *module;
    struct Map_info Map, Out;
    int *testnode;		/* array all nodes: 1 - should be tested as Steiner, 
				 * 0 - no need to test (unreachable or terminal) */
    struct ilist *TList;	/* list of terminal nodes */
    struct ilist *StArcs;	/* list of arcs on Steiner tree */
    struct ilist *StNodes;	/* list of nodes on Steiner tree */
    struct boxlist *pointlist;
    double cost, tmpcost;
    struct cat_list *Clist;
    struct line_cats *Cats;
    struct line_pnts *Points;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("steiner tree"));
    module->label =
	_("Creates Steiner tree for the network and given terminals.");
    module->description =
	_("Note that 'Minimum Steiner Tree' problem is NP-hard "
	  "and heuristic algorithm is used in this module so "
	  "the result may be sub optimal.");

    map = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";
    type_opt->label = _("Arc type");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");

    tfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    tfield_opt->key = "nlayer";
    tfield_opt->answer = "2";
    tfield_opt->label = _("Node layer (used for terminals)");

    afcol = G_define_option();
    afcol->key = "acolumn";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description = _("Arcs' cost column (for both directions)");

    term_opt = G_define_standard_option(G_OPT_V_CATS);
    term_opt->key = "tcats";
    term_opt->required = YES;
    term_opt->description =
	_("Categories of points on terminals (layer is specified by nlayer)");

    nsp_opt = G_define_option();
    nsp_opt->key = "nsp";
    nsp_opt->type = TYPE_INTEGER;
    nsp_opt->required = NO;
    nsp_opt->multiple = NO;
    nsp_opt->answer = "-1";
    nsp_opt->description = _("Number of steiner points (-1 for all possible)");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();

    type = Vect_option_to_types(type_opt);
    afield = atoi(afield_opt->answer);

    TList = Vect_new_list();
    StArcs = Vect_new_list();
    StNodes = Vect_new_list();

    Clist = Vect_new_cat_list();
    tfield = atoi(tfield_opt->answer);
    Vect_str_to_cat_list(term_opt->answer, Clist);

    G_debug(1, "Imput categories:\n");
    for (i = 0; i < Clist->n_ranges; i++) {
	G_debug(1, "%d - %d\n", Clist->min[i], Clist->max[i]);
    }

    if (geo_f->answer)
	geo = 1;
    else
	geo = 0;

    Vect_check_input_output_name(map->answer, output->answer, G_FATAL_EXIT);

    Vect_set_open_level(2);
    Vect_open_old(&Map, map->answer, "");
    nnodes = Vect_get_num_nodes(&Map);
    nlines = Vect_get_num_lines(&Map);

    /* Create list of terminals based on list of categories */
    for (i = 1; i <= nlines; i++) {
	ltype = Vect_get_line_type(&Map, i);
	if (!(ltype & GV_POINT))
	    continue;

	Vect_read_line(&Map, Points, Cats, i);
	node = Vect_find_node(&Map, Points->x[0], Points->y[0], Points->z[0], 0, 0);
	if (!node) {
	    G_warning(_("Point is not connected to the network"));
	    continue;
	}
	if (!(Vect_cat_get(Cats, tfield, &cat)))
	    continue;
	if (Vect_cat_in_cat_list(cat, Clist)) {
	    Vect_list_append(TList, i);
	}
    }

    nterms = TList->n_values;
    fprintf(stdout, "Number of terminals: %d\n", nterms);

    if (nterms < 2)
	G_fatal_error(_("Not enough terminals (< 2)"));

    /* Number of steiner points */
    nsp = atoi(nsp_opt->answer);
    if (nsp > nterms - 2) {
	nsp = nterms - 2;
	G_warning(_("Requested number of Steiner points > than possible"));
    }
    else if (nsp == -1) {
	nsp = nterms - 2;
    }

    fprintf(stdout, "Number of Steiner points set to %d\n", nsp);

    testnode = (int *)G_malloc((nnodes + 1) * sizeof(int));
    for (i = 1; i <= nnodes; i++)
	testnode[i] = 1;

    /* Alloc arrays of costs for nodes, first node at 1 (0 not used) */
    nodes_costs = (double **)G_malloc((nnodes - 1) * sizeof(double *));
    for (i = 0; i < nnodes; i++) {
	nodes_costs[i] =
	    (double *)G_malloc((nnodes - i - 1) * sizeof(double));
	for (j = 0; j < nnodes - i - 1; j++)
	    nodes_costs[i][j] = -1;	/* init, i.e. cost was not calculated yet */
    }

    /* alloc memory from each to each other (not directed) terminal */
    i = nterms + nterms - 2;	/*  max number of terms + Steiner points */
    comps = (int *)G_malloc(i * sizeof(int));
    i = i * (i - 1) / 2;	/* number of combinations */
    term_costs = (COST *) G_malloc(i * sizeof(COST));

    /* alloc memory for costs from Stp to each other terminal */
    i = nterms + nterms - 2 - 1;	/*  max number of terms + Steiner points - 1 */
    sp_costs = (COST *) G_malloc(i * sizeof(COST));

    terms = (int *)G_malloc((nterms + nterms - 2) * sizeof(int));	/* i.e. +(nterms - 2)  St Points */
    /* Create initial parts from list of terminals */
    G_debug(1, "List of terminal nodes (%d):\n", nterms);
    for (i = 0; i < nterms; i++) {
	G_debug(1, "%d\n", TList->value[i]);
	terms[i] = TList->value[i];
	testnode[terms[i]] = 0;	/* do not test as Steiner */
    }

    /* Build graph */
    Vect_net_build_graph(&Map, type, afield, 0, afcol->answer, NULL, NULL,
			 geo, 0);

    /* Init costs for all terminals */
    for (i = 0; i < nterms; i++)
	init_node_costs(&Map, terms[i]);

    /* Test if all terminal may be connected */
    for (i = 1; i < nterms; i++) {
	ret = get_node_costs(terms[0], terms[i], &cost);
	if (ret == 0) {
	    G_fatal_error(_("Terminal at node [%d] cannot be connected "
			    "to terminal at node [%d]"), terms[0], terms[i]);
	}
    }

    /* Remove not reachable from list of SP candidates */
    j = 0;
    for (i = 1; i <= nnodes; i++) {
	ret = get_node_costs(terms[0], i, &cost);
	if (ret == 0) {
	    testnode[i] = 0;
	    /* fprintf (stderr, "node %d removed from list of Steiner point candidates\n", i ); */
	    j++;
	}
    }

    G_message(_("[%d] (not reachable) nodes removed from list "
		"of Steiner point candidates"), j);

    /* calc costs for terminals MST */
    ret = mst(&Map, terms, nterms, &cost, PORT_DOUBLE_MAX, NULL, NULL, 0, 1);	/* no StP, rebuild */
    G_message(_("MST costs = %f"), cost);

    /* Go through all nodes and try to use as steiner points -> find that which saves most costs */
    nspused = 0;
    for (j = 0; j < nsp; j++) {
	sp = 0;
	G_message(_("Search for [%d]. Steiner point"), j + 1);

	for (i = 1; i <= nnodes; i++) {
	    G_percent(i, nnodes, 1);
	    if (testnode[i] == 0) {
		G_debug(3, "skip test for %d\n", i);
		continue;
	    }
	    ret =
		mst(&Map, terms, nterms + j, &tmpcost, cost, NULL, NULL, i,
		    0);
	    G_debug(2, "cost = %f x %f\n", tmpcost, cost);
	    if (tmpcost < cost) {	/* sp candidate */
		G_debug(3,
			"  steiner candidate node = %d mst = %f (x last = %f)\n",
			i, tmpcost, cost);
		sp = i;
		cost = tmpcost;
	    }
	}
	if (sp > 0) {
	    G_message(_("Steiner point at node [%d] was added "
			"to terminals (MST costs = %f)"), sp, cost);
	    terms[nterms + j] = sp;
	    init_node_costs(&Map, sp);
	    testnode[sp] = 0;
	    nspused++;
	    /* rebuild for nex cycle */
	    ret =
		mst(&Map, terms, nterms + nspused, &tmpcost, PORT_DOUBLE_MAX,
		    NULL, NULL, 0, 1);
	}
	else {			/* no steiner found */
	    G_message(_("No Steiner point found -> leaving cycle"));
	    break;
	}
    }

    fprintf(stdout, "\nNumber of added Steiner points: %d "
	    "(theoretic max is %d).\n", nspused, nterms - 2);

    /* Build lists of arcs and nodes for final version */
    ret =
	mst(&Map, terms, nterms + nspused, &cost, PORT_DOUBLE_MAX, StArcs,
	    StNodes, 0, 0);

    /* Calculate true costs, which may be lower than MST if steiner points were not used */

    if (nsp < nterms - 2) {
	fprintf(stdout, "\nSpanning tree costs on complet graph = %f\n"
		"(may be higher than resulting Steiner tree costs!!!)\n",
		cost);
    }
    else
	fprintf(stdout, "\nSteiner tree costs = %f\n", cost);

    /* Write arcs to new map */
    Vect_open_new(&Out, output->answer, Vect_is_3d(&Map));
    Vect_hist_command(&Out);

    fprintf(stdout, "\nSteiner tree:\n");
    fprintf(stdout, "Arcs' categories (layer %d, %d arcs):\n", afield,
	    StArcs->n_values);

    for (i = 0; i < StArcs->n_values; i++) {
	line = StArcs->value[i];
	ltype = Vect_read_line(&Map, Points, Cats, line);
	Vect_write_line(&Out, ltype, Points, Cats);
	Vect_cat_get(Cats, afield, &cat);
	if (i > 0)
	    printf(",");
	fprintf(stdout, "%d", cat);
    }

    fprintf(stdout, "\n\n");

    fprintf(stdout, "Nodes' categories (layer %d, %d nodes):\n", tfield,
	    StNodes->n_values);

    k = 0;
    pointlist = Vect_new_boxlist(0);
    for (i = 0; i < StNodes->n_values; i++) {
	double x, y, z;
	struct bound_box box;
	
	node = StNodes->value[i];
	
	Vect_get_node_coor(&Map, node, &x, &y, &z);
	box.E = box.W = x;
	box.N = box.S = y;
	box.T = box.B = z;
	Vect_select_lines_by_box(&Map, &box, GV_POINT, pointlist);
	
	nlines = Vect_get_node_n_lines(&Map, node);
	for (j = 0; j < pointlist->n_values; j++) {
	    line = pointlist->id[j];
	    ltype = Vect_read_line(&Map, Points, Cats, line);
	    if (!(ltype & GV_POINT))
		continue;
	    if (!(Vect_cat_get(Cats, tfield, &cat)))
		continue;
	    Vect_write_line(&Out, ltype, Points, Cats);
	    if (k > 0)
		fprintf(stdout, ",");
	    fprintf(stdout, "%d", cat);
	    k++;
	}
    }

    fprintf(stdout, "\n\n");

    Vect_build(&Out);

    /* Free, ... */
    Vect_destroy_list(StArcs);
    Vect_destroy_list(StNodes);
    Vect_close(&Map);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}

int cmp(const void *pa, const void *pb)
{
    COST *p1 = (COST *) pa;
    COST *p2 = (COST *) pb;

    if (p1->cost < p2->cost)
	return -1;

    if (p1->cost > p2->cost)
	return 1;

    return 0;
}
