#ifndef GRASS_IMAGEDEFS_H
#define GRASS_IMAGEDEFS_H

/* alloc.c */
void *I_malloc(size_t);
void *I_realloc(void *, size_t);
int I_free(void *);
double **I_alloc_double2(int, int);
int *I_alloc_int(int);
int **I_alloc_int2(int, int);
int I_free_int2(int **);
int I_free_double2(double **);
double ***I_alloc_double3(int, int, int);
int I_free_double3(double ***);

/* eol.c */
int I_get_to_eol(char *, int, FILE *);

/* find.c */
int I_find_group(const char *);
int I_find_group_file(const char *, const char *);
int I_find_subgroup(const char *, const char *);
int I_find_subgroup_file(const char *, const char *, const char *);

/* fopen.c */
FILE *I_fopen_group_file_new(const char *, const char *);
FILE *I_fopen_group_file_append(const char *, const char *);
FILE *I_fopen_group_file_old(const char *, const char *);
FILE *I_fopen_subgroup_file_new(const char *, const char *, const char *);
FILE *I_fopen_subgroup_file_append(const char *, const char *, const char *);
FILE *I_fopen_subgroup_file_old(const char *, const char *, const char *);

/* georef.c */
int I_compute_georef_equations(struct Control_Points *, double *, double *,
			       double *, double *, int);
int I_georef(double, double, double *, double *, double *, double *, int);

/* georef_tps.c */
int I_compute_georef_equations_tps(struct Control_Points *, double **, double **,
			       double **, double **);
int I_georef_tps(double, double, double *, double *, double *, double *, 
                 struct Control_Points *, int);

/* group.c */
int I_get_group(char *);
int I_put_group(const char *);
int I_get_subgroup(const char *, char *);
int I_put_subgroup(const char *, const char *);
int I_get_group_ref(const char *, struct Ref *);
int I_get_subgroup_ref(const char *, const char *, struct Ref *);
int I_init_ref_color_nums(struct Ref *);
int I_put_group_ref(const char *, const struct Ref *);
int I_put_subgroup_ref(const char *, const char *, const struct Ref *);
int I_add_file_to_group_ref(const char *, const char *, struct Ref *);
int I_transfer_group_ref_file(const struct Ref *, int, struct Ref *);
int I_init_group_ref(struct Ref *);
int I_free_group_ref(struct Ref *);

/* iclass.c */
struct Map_info;
int I_iclass_analysis(IClass_statistics *, struct Ref *, struct Map_info *, const char *, const char *, const char *);
int I_iclass_init_group(const char *, const char *, struct Ref *);
void I_iclass_create_raster(IClass_statistics *, struct Ref *, const char *);

/* iclass_statistics.c */
void I_iclass_statistics_get_nbands(IClass_statistics *, int *);
void I_iclass_statistics_get_cat(IClass_statistics *, int *);
void I_iclass_statistics_get_name(IClass_statistics *, const char **);
void I_iclass_statistics_get_color(IClass_statistics *, const char **);
void I_iclass_statistics_get_ncells(IClass_statistics *, int *);
int I_iclass_statistics_get_max(IClass_statistics *, int, int *);
int I_iclass_statistics_get_range_max(IClass_statistics *, int, int *);
int I_iclass_statistics_get_min(IClass_statistics *, int, int *);
int I_iclass_statistics_get_range_min(IClass_statistics *, int, int *);
int I_iclass_statistics_get_sum(IClass_statistics *, int, float *);
int I_iclass_statistics_get_mean(IClass_statistics *, int, float *);
int I_iclass_statistics_get_stddev(IClass_statistics *, int, float *);
void I_iclass_statistics_get_nstd(IClass_statistics *, float *);
void I_iclass_statistics_set_nstd(IClass_statistics *, float);
int I_iclass_statistics_get_histo(IClass_statistics *, int, int, int *);
int I_iclass_statistics_get_product(IClass_statistics *, int, int, float *);
void I_iclass_init_statistics(IClass_statistics *, int, const char *, const char *, float);
void I_iclass_free_statistics(IClass_statistics *);

/* iclass_signatures.c */
int I_iclass_init_signatures(struct Signature *, struct Ref *);
void I_iclass_add_signature(struct Signature *, IClass_statistics *);
int I_iclass_write_signatures(struct Signature *, const char *, const char *, const char *);

/* list_gp.c */
int I_list_group(const char *, const struct Ref *, FILE *);
int I_list_group_simple(const struct Ref *, FILE *);

/* list_subgp.c */
char ** I_list_subgroups(const char *, int *);
int I_list_subgroup(const char *, const char *, const struct Ref *, FILE *);
int I_list_subgroup_simple(const struct Ref *, FILE *);

/* loc_info.c */
char *I_location_info(const char *);

/* points.c */
int I_new_control_point(struct Control_Points *, double, double, double,
			double, int);
int I_get_control_points(const char *, struct Control_Points *);
int I_put_control_points(const char *, const struct Control_Points *);

/* ref.c */
FILE *I_fopen_group_ref_new(const char *);
FILE *I_fopen_group_ref_old(const char *);
FILE *I_fopen_subgroup_ref_new(const char *, const char *);
FILE *I_fopen_subgroup_ref_old(const char *, const char *);

/* sig.c */
int I_init_signatures(struct Signature *, int);
int I_new_signature(struct Signature *);
int I_free_signatures(struct Signature *);
int I_read_one_signature(FILE *, struct Signature *);
int I_read_signatures(FILE *, struct Signature *);
int I_write_signatures(FILE *, struct Signature *);

/* sigfile.c */
FILE *I_fopen_signature_file_new(const char *, const char *, const char *);
FILE *I_fopen_signature_file_old(const char *, const char *, const char *);

/* sigset.c */
int I_SigSetNClasses(struct SigSet *);
struct ClassData *I_AllocClassData(struct SigSet *, struct ClassSig *, int);
int I_InitSigSet(struct SigSet *);
int I_SigSetNBands(struct SigSet *, int);
struct ClassSig *I_NewClassSig(struct SigSet *);
struct SubSig *I_NewSubSig(struct SigSet *, struct ClassSig *);
int I_ReadSigSet(FILE *, struct SigSet *);
int I_SetSigTitle(struct SigSet *, const char *);
const char *I_GetSigTitle(const struct SigSet *);
int I_SetClassTitle(struct ClassSig *, const char *);
const char *I_GetClassTitle(const struct ClassSig *);
int I_WriteSigSet(FILE *, const struct SigSet *);

/* sigsetfile.c */
FILE *I_fopen_sigset_file_new(const char *, const char *, const char *);
FILE *I_fopen_sigset_file_old(const char *, const char *, const char *);

/* target.c */
int I_get_target(const char *, char *, char *);
int I_put_target(const char *, const char *, const char *);

/* title.c */
int I_get_group_title(const char *, char *, int);
int I_put_group_title(const char *, const char *);

/* var.c */
double I_variance(double, double, int);
double I_stddev(double, double, int);

#endif
