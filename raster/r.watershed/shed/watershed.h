#ifndef __WATERSHED_H__
#define __WATERSHED_H__


#include <stdio.h>
#include <grass/gis.h>

#define RAM_NAME		"ram"
#define SEG_NAME		"seg"
#define NON_NAME		"watershed"
#define ACRE_TO_METERSQ		4047.0
#define MILESQ_TO_ACRE		640.0
#define HECTACRE_TO_METERSQ	10000.0
#define KILOSQ_TO_ACRE		247.1
#define KILOSQ_TO_METERSQ	1000000.0
#define MILESQ_TO_METERSQ	2590080.0
#define METERSQ_TO_ACRE		0.00024709661
#define METERSQ_TO_MILESQ	0.00000038608
#define METERSQ_TO_HECTACRE	0.0001
#define METERSQ_TO_KILOSQ	0.000001
#define METER_TO_FOOT		3.281
#define INCR			32
#define INPUT struct inputtttttt
#define OUTPUT struct outputttttt
#define BASIN struct mapvallllllll
#define MAP struct mapssssss
#define B_FACTS struct bfactsssssss
#define CAT struct cattttttttttt

INPUT {
    char *ar_file_name, *com_line_ram, *com_line_seg, *haf_name, *accum_name;
    char fast, slow;
};

CAT {
    int num_cat;		/* num of cells */
    CELL cat_val;
    CAT *nxt;
};

BASIN {
    CAT first_cat;		/* linked list of cats with num */
    double sum_values;		/* summation */
};

MAP {
    char *name, *mapset;
    BASIN *basins;		/* array of basins */
    struct Categories cats;
    char do_cats;
};

OUTPUT {
    MAP *maps;			/* map layers of output stuff */
    B_FACTS *basin_facts;	/* basin information array */
    FILE *out_file;
    struct Cell_head window;
    int num_maps, num_basins;

    /* output file, display map name, flag for basin by percent, flag for accum of percent */
    char *file_name, do_basin, do_accum, type_area;
};

B_FACTS {
    double str_length, str_slope, accum_length, accum_slope, easting,
	northing;
    int num_cells, down_basin, valid;
};


/* accum_down.c */
int accum_down(OUTPUT *);

/* basin_maps.c */
int basin_maps(INPUT *, OUTPUT *);

/* com_line.c */
int com_line_Gwater(INPUT *, OUTPUT *);
int com_line_add(char **, char *, char *, char *);
int basin_com_add(char **, double, double, struct Cell_head *);
int com_add(char **, char *, int);

/* file_in.c */
int ar_file_in(char *, OUTPUT *);

/* free.c */
int free_input(INPUT *);
int free_output(OUTPUT *);

/* insert_cat.c */
int insert_cat(CAT *, CELL, int);
CAT *new_cat(CELL, int);

/* intro.c */
int intro(void);

/* print.c */
int print_output(OUTPUT *);

/* read.c */
int read_basins(char *, OUTPUT *);

/* valid.c */
int valid_basins(char *, OUTPUT *);


#endif /* __WATERSHED_H__ */
