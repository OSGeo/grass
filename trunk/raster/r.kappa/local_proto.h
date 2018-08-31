/* calc_kappa.c */
void calc_kappa(void);

/* mask.c */
char *maskinfo(void);

/* prt_hdr.c */
void prn_header(void);

/* prt_label.c */
void prt_label(void);

/* prt_mat.c */
void prn_error_mat(int out_cols, int hdr);

/* prt2csv_mat.c */
void prn2csv_error_mat(int out_cols, int hdr);

/* stats.c */
int stats(void);

/* sum.c */
long count_sum(int *ns, int n1);
