struct grid_description
{
    int num_rows;
    int num_cols;
    int num_vect_rows;
    int num_vect_cols;
    double length;		/*  distance to shift to the east  */
    double width;		/*  distance to shift to the north  */
    double origin_x;		/*  lower left point of grid  */
    double origin_y;
    double angle;
};
