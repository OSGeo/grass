/* This file contains the declarations of all generalization operators
 * implemented in this v.generalize */

/* simplification.c */
int douglas_peucker(struct line_pnts *Points, double thresh, int with_z);
int lang(struct line_pnts *Points, double thresh, int look_ahead, int with_z);
int vertex_reduction(struct line_pnts *Points, double eps, int with_z);
int reumann_witkam(struct line_pnts *Points, double thresh, int with_z);
int douglas_peucker_reduction(struct line_pnts *Points, double thresh,
			      double reduction, int with_z);

/* smoothing.c */
int boyle(struct line_pnts *Points, int look_ahead, int with_z);
int sliding_averaging(struct line_pnts *Points, double slide, int look_ahead,
		      int loop_support, int with_z);
int distance_weighting(struct line_pnts *Points, double slide, int look_ahead,
		       int with_z);
int chaiken(struct line_pnts *Points, double thresh, int with_z);
int hermite(struct line_pnts *Points, double step, double angle_thresh,
	    int with_z);
int snakes(struct line_pnts *Points, double alpha, double beta, int with_z);

/* network.c */
int graph_generalization(struct Map_info *In, struct Map_info *Out, 
                         int type, double degree_thresh, 
			 double closeness_thresh, double betweeness_thresh);

/* displacement.c */
int snakes_displacement(struct Map_info *In, struct Map_info *Out,
			double threshold, double alpha, double beta,
			double gama, double delta, int iterations,
			struct cat_list *cat_list, int layer);
