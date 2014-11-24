struct grid_description
{
    int num_rows;
    int num_cols;
    int num_vect_rows;
    int num_vect_cols;
    double width;		/* grid cell width (EW res) */
    double height;		/* grid cell height (NS res) */
    double north, south, east, west;
    double xo, yo;		/* grid origin = center */
    double angle;
    /* hexagon */
    double rrad, crad;
    double rstep, cstep;
};
