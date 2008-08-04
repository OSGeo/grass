/* Prototypes from src/mapdev/v.build */
/* area.c */
int build_all_areas(struct Map_info *, struct Map_info *);
int build_area(struct Map_info *, int);
int check_area(struct Map_info *, int, double, double);

/* ascii_io.c */
char codes(int);
int Wr_P_node_asc(int, struct P_node *, FILE *);
int Wr_P_line_asc(int, struct P_line *, FILE *);
int Wr_P_area_asc(int, struct P_area *, FILE *);
int Wr_P_att_asc(int, struct P_att *, FILE *);
int Wr_P_isle_asc(int, struct P_isle *, FILE *);
int Wr_Plus_head_asc(struct Plus_head *, FILE *);

/* atts_file.c */
int clean_atts_file(char *);
int cp_file(char *, char *);

/* b_a_plus.c */
int main(int, char *[]);

/* find_nodes.c */
int find_nodes(double, double, int, struct P_node *, double);

/* import_line.c */
int import_line(struct Map_info *, int, struct new_node *, struct line_pnts *,
		long);
/* init_plus_s.c */
int init_plus_struct(struct Plus_head *);
int init_map_struct(struct Map_info *);

/* isle.c */
int matchup_isles(struct Map_info *);
int matchup_isle(struct Map_info *, register int);

/* labels.c */
int read_atts(struct Map_info *, char *);

/* main.c */
int debugf(char *, ...);

/* open_files.c */
int open_dig_files(char *, FILE **, struct Map_info *, struct Plus_head *);

/* read_digit.c */
int read_digit(struct Map_info *, struct Plus_head *);
int init_extents(void);
int init_extents_from_head(struct dig_head *);
int update_head_from_ext(struct dig_head *);

/* write_ascii.c */
int write_plus_asc(struct Map_info *, FILE *, FILE *);
int write_nodes_asc(struct Map_info *, FILE *, FILE *, struct Plus_head *);
int write_lines_asc(struct Map_info *, FILE *, FILE *, struct Plus_head *);
int write_areas_asc(struct Map_info *, FILE *, FILE *, struct Plus_head *);
int write_atts_asc(struct Map_info *, FILE *, FILE *, struct Plus_head *);
int write_isles_asc(struct Map_info *, FILE *, FILE *, struct Plus_head *);
