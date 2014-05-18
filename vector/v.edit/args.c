#include "global.h"

/**
   \brief Parser stuff

   \param[in] argc number of arguments
   \param[in] argv arguments array
   \param[in] params GRASS paramenters
   \param[out] action mode selected tool

   \return 1
*/
int parser(int argc, char *argv[], struct GParams *params,
	   enum mode *action_mode)
{
    char *desc_tool, *desc_query, *desc_snap;

    /* parameters */
    params->map = G_define_standard_option(G_OPT_V_MAP);
    params->map->label = _("Name of vector map to edit");

    params->fld = G_define_standard_option(G_OPT_V_FIELD);
    params->fld->gisprompt = "old,layer,layer";
    params->fld->guisection = _("Selection");

    params->type = G_define_standard_option(G_OPT_V_TYPE);
    params->type->options = "point,line,boundary,centroid";
    params->type->answer = "point,line,boundary,centroid";
    params->type->guisection = _("Selection");

    params->tool = G_define_option();
    params->tool->key = "tool";
    params->tool->type = TYPE_STRING;
    params->tool->required = YES;
    params->tool->multiple = NO;
    params->tool->description = _("Tool");
    desc_tool = NULL;
    G_asprintf(&desc_tool,
	       "create;%s;"
	       "add;%s;"
	       "delete;%s;"
	       "move;%s;"
	       "vertexmove;%s;"
	       "vertexdel;%s;"
	       "vertexadd;%s;"
	       "merge;%s;"
	       "break;%s;"
	       "select;%s;"
	       "catadd;%s;"
	       "catdel;%s;"
	       "copy;%s;"
	       "snap;%s;"
	       "flip;%s;"
	       "connect;%s;"
	       "zbulk;%s;"
	       "chtype;%s;"
	       "areadel;%s",
	       _("Create new (empty) vector map"),
	       _("Add new features to existing vector map"),
	       _("Delete selected features from vector map"),
	       _("Move selected features in vector map"),
	       _("Move vertex of selected vector lines"),
	       _("Remove vertex from selected vector lines"),
	       _("Add new vertex to selected vector lines"),
	       _("Merge selected vector lines"),
	       _("Break/split vector lines"),
	       _("Select lines and print their ID's"),
	       _("Set new categories to selected vector features "
		 "for defined layer"),
	       _("Delete categories from selected vector features "
		 "for defined layer"),
	       _("Copy selected features"),
	       _("Snap vector features in given threshold"),
	       _("Flip direction of selected vector lines"),
	       _("Connect two lines"),
	       _("Z bulk-labeling (automated assignment of z coordinate to "
		 "vector lines)"),
	       _("Change feature type (point<->centroid, line<->boundary)"),
	       _("Delete selected areas from vector map (based on selected centroids)"));
    params->tool->descriptions = desc_tool;
    params->tool->options = "create,add,delete,copy,move,flip,catadd,catdel,"
	"merge,break,snap,connect,chtype,"
	"vertexadd,vertexdel,vertexmove,areadel,zbulk,select";

    params->in = G_define_standard_option(G_OPT_F_INPUT);
    params->in->required = NO;
    params->in->label = _("Name of file containing data in GRASS ASCII vector format");
    params->in->description =
	_("\"-\" reads from standard input");
    params->in->guisection = _("Input");

    params->move = G_define_option();
    params->move->key = "move";
    params->move->key_desc = "x,y,z";
    params->move->type = TYPE_DOUBLE;
    params->move->required = NO;
    params->move->multiple = NO;
    params->move->description =
	_("Difference in x,y,z direction for moving feature or vertex");

    params->maxdist = G_define_option();
    params->maxdist->key = "thresh";
    params->maxdist->type = TYPE_DOUBLE;
    params->maxdist->required = NO;
    params->maxdist->multiple = YES;
    params->maxdist->label = _("Threshold distance (coords,snap,query)");
    params->maxdist->description =
	_("'-1' for threshold based on the current resolution settings");
    params->maxdist->answer = "-1,0,0";

    params->id = G_define_standard_option(G_OPT_V_IDS);
    params->id->guisection = _("Selection");

    params->cat = G_define_standard_option(G_OPT_V_CATS);
    params->cat->required = NO;
    params->cat->guisection = _("Selection");

    params->coord = G_define_option();
    params->coord->key = "coords";
    params->coord->key_desc = "x,y";
    params->coord->type = TYPE_DOUBLE;
    params->coord->required = NO;
    params->coord->multiple = YES;
    params->coord->description = _("List of point coordinates");
    params->coord->guisection = _("Selection");

    params->bbox = G_define_option();
    params->bbox->key = "bbox";
    params->bbox->key_desc = "x1,y1,x2,y2";
    params->bbox->type = TYPE_DOUBLE;
    params->bbox->required = NO;
    params->bbox->multiple = NO;
    params->bbox->description = _("Bounding box for selecting features");
    params->bbox->guisection = _("Selection");

    params->poly = G_define_option();
    params->poly->key = "polygon";
    params->poly->key_desc = "x,y";
    params->poly->type = TYPE_DOUBLE;
    params->poly->required = NO;
    params->poly->multiple = YES;
    params->poly->description = _("Polygon for selecting features");
    params->poly->guisection = _("Selection");

    params->where = G_define_standard_option(G_OPT_DB_WHERE);
    params->where->guisection = _("Selection");

    params->query = G_define_option();
    params->query->key = "query";
    params->query->type = TYPE_STRING;
    params->query->options = "length,dangle";
    params->query->label = _("Query tool");
    params->query->description =
	_("For 'shorter' use negative threshold value, "
	  "positive value for 'longer'");
    desc_query = NULL;
    G_asprintf(&desc_query,
               "length;%s;"
	       "dangle;%s",
	       _("Select only lines or boundaries shorter"
		 "/longer than threshold distance"),
	       _("Select dangles shorter/longer than threshold distance"));
    params->query->descriptions = desc_query;
    params->query->guisection = _("Selection");

    params->bmaps = G_define_standard_option(G_OPT_V_MAPS);
    params->bmaps->key = "bgmap";
    params->bmaps->required = NO;
    params->bmaps->description = _("Name of background vector map(s)");

    params->snap = G_define_option();
    params->snap->key = "snap";
    params->snap->type = TYPE_STRING;
    params->snap->options = "no,node,vertex";
    params->snap->description =
	_("Snap added or modified features in the given threshold to the nearest existing feature");
    desc_snap = NULL;
    G_asprintf(&desc_snap,
	       "no;%s;"
	       "node;%s;"
	       "vertex;%s",
	       _("Not apply snapping"),
	       _("Snap only to node"),
	       _("Allow snapping also to vertex"));
    params->snap->descriptions = desc_snap;
    params->snap->answer = "no";

    params->zbulk = G_define_option();
    params->zbulk->key = "zbulk";
    params->zbulk->type = TYPE_DOUBLE;
    params->zbulk->key_desc = "value,step";
    params->zbulk->label = _("Starting value and step for z bulk-labeling");
    params->zbulk->description = _("Pair: value,step (e.g. 1100,10)");

    /* flags */
    params->reverse = G_define_flag();
    params->reverse->key = 'r';
    params->reverse->description = _("Reverse selection");
    params->reverse->guisection = _("Selection");

    params->close = G_define_flag();
    params->close->key = 'c';
    params->close->description =
	_("Close added boundaries (using threshold distance)");

    params->header = G_define_flag();
    params->header->key = 'n';
    params->header->description = _("Do not expect header of input data");
    params->header->guisection = _("Input");

    params->topo = G_define_standard_flag(G_FLG_V_TOPO);

    params->move_first = G_define_flag();
    params->move_first->key = '1';
    params->move_first->description =
	_("Modify only first found feature in bounding box");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* check for polygon param */
    if (params->poly->answers != NULL) {
	int i = 0;

	while (params->poly->answers[i++]) ;

	if (i < 6)
	    G_fatal_error(_("Polygon must have at least 3 coordinate pairs"));
    }

    /*
       check that the given arguments makes sense together
     */
    if (strcmp(params->tool->answer, "create") == 0) {
	*action_mode = MODE_CREATE;
    }
    else if (strcmp(params->tool->answer, "add") == 0) {
	*action_mode = MODE_ADD;
    }
    else if (strcmp(params->tool->answer, "delete") == 0) {
	*action_mode = MODE_DEL;
    }
    else if (strcmp(params->tool->answer, "move") == 0) {
	*action_mode = MODE_MOVE;
    }
    else if (strcmp(params->tool->answer, "merge") == 0) {
	*action_mode = MODE_MERGE;
    }
    else if (strcmp(params->tool->answer, "break") == 0) {
	*action_mode = MODE_BREAK;
    }
    else if (strcmp(params->tool->answer, "connect") == 0) {
	*action_mode = MODE_CONNECT;
    }
    else if (strcmp(params->tool->answer, "vertexadd") == 0) {
	*action_mode = MODE_VERTEX_ADD;
    }
    else if (strcmp(params->tool->answer, "vertexdel") == 0) {
	*action_mode = MODE_VERTEX_DELETE;
    }
    else if (strcmp(params->tool->answer, "vertexmove") == 0) {
	*action_mode = MODE_VERTEX_MOVE;
    }
    else if (strcmp(params->tool->answer, "select") == 0) {
	*action_mode = MODE_SELECT;
    }
    else if (strcmp(params->tool->answer, "catadd") == 0) {
	*action_mode = MODE_CATADD;
    }
    else if (strcmp(params->tool->answer, "catdel") == 0) {
	*action_mode = MODE_CATDEL;
    }
    else if (strcmp(params->tool->answer, "copy") == 0) {
	*action_mode = MODE_COPY;
    }
    else if (strcmp(params->tool->answer, "snap") == 0) {
	*action_mode = MODE_SNAP;
    }
    else if (strcmp(params->tool->answer, "flip") == 0) {
	*action_mode = MODE_FLIP;
    }
    else if (strcmp(params->tool->answer, "zbulk") == 0) {
	*action_mode = MODE_ZBULK;
    }
    else if (strcmp(params->tool->answer, "chtype") == 0) {
	*action_mode = MODE_CHTYPE;
    }
    else if (strcmp(params->tool->answer, "areadel") == 0) {
	*action_mode = MODE_AREA_DEL;
    }
    else {
	G_fatal_error(_("Operation '%s' not implemented"),
		      params->tool->answer);
    }

    if ((*action_mode != MODE_CREATE && *action_mode != MODE_ADD &&
	 *action_mode != MODE_ZBULK) && (params->cat->answers == NULL) &&
	(params->coord->answers == NULL) && (params->poly->answers == NULL) &&
	(params->id->answers == NULL) && (params->bbox->answers == NULL) &&
	(params->where->answer == NULL) && (params->query->answer == NULL)) {
	G_fatal_error(_("At least one option from %s must be specified"),
		      "cats, ids, coords, bbox, polygon, where, query");
    }

    if (*action_mode == MODE_MOVE || *action_mode == MODE_VERTEX_MOVE) {
	if (params->move->answers == NULL) {
	    G_fatal_error(_("Tool %s requires option %s"),
			  params->tool->answer, params->move->key);
	}
    }

    if (*action_mode == MODE_VERTEX_ADD ||
	*action_mode == MODE_VERTEX_DELETE ||
	*action_mode == MODE_VERTEX_MOVE) {
	if (params->coord->answers == NULL) {
	    G_fatal_error(_("Tool %s requires option %s"),
			  params->tool->answer, params->coord->key);
	}
    }

    if (*action_mode == MODE_CATADD || *action_mode == MODE_CATDEL) {
	if (params->cat->answers == NULL) {
	    G_fatal_error(_("Tool %s requires option %s"),
			  params->tool->answer, params->cat->key);
	}
    }

    if (*action_mode == MODE_ZBULK) {
	if (params->bbox->answers == NULL) {
	    G_fatal_error(_("Tool %s requires option %s"),
			  params->tool->answer, params->bbox->key);
	}
	if (params->zbulk->answers == NULL) {
	    G_fatal_error(_("Tool %s requires option %s"),
			  params->tool->answer, params->zbulk->key);
	}
    }

    return 1;
}
