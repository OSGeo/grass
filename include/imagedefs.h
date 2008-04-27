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
/* ask_group.c */
int I_ask_group_old(char *, char *);
/* eol.c */
int I_get_to_eol(char *, int, FILE *);
/* find.c */
int I_find_group(char *);
int I_find_group_file(char *, char *);
int I_find_subgroup(char *, char *);
int I_find_subgroup_file(char *, char *, char *);
/* fopen.c */
FILE *I_fopen_group_file_new(char *, char *);
FILE *I_fopen_group_file_append(char *, char *);
FILE *I_fopen_group_file_old(char *, char *);
FILE *I_fopen_subgroup_file_new(char *, char *, char *);
FILE *I_fopen_subgroup_file_append(char *, char *, char *);
FILE *I_fopen_subgroup_file_old(char *, char *, char *);
/* georef.c */
int I_compute_georef_equations(struct Control_Points *, double [3], double [3], double [3], double [3]);
int I_georef(double, double, double *, double *, double [3], double [3]);
/* group.c */
int I_get_group(char *);
int I_put_group(char *);
int I_get_subgroup(char *, char *);
int I_put_subgroup(char *, char *);
int I_get_group_ref(char *, struct Ref *);
int I_get_subgroup_ref(char *, char *, struct Ref *);
int I_init_ref_color_nums(struct Ref *);
int I_put_group_ref(char *, struct Ref *);
int I_put_subgroup_ref(char *, char *, struct Ref *);
int I_add_file_to_group_ref(char *, char *, struct Ref *);
int I_transfer_group_ref_file(struct Ref *, int, struct Ref *);
int I_init_group_ref(struct Ref *);
int I_free_group_ref(struct Ref *);
/* list_gp.c */
int I_list_group(char *, struct Ref *, FILE *);
int I_list_group_simple(struct Ref *, FILE *);
/* list_subgp.c */
int I_list_subgroup(char *, char *, struct Ref *, FILE *);
int I_list_subgroup_simple(struct Ref *, FILE *);
/* loc_info.c */
int I_location_info(char *, char *);
/* ls_groups.c */
int I_list_groups(int);
int I_list_subgroups(char *, int);
/* points.c */
int I_new_control_point(struct Control_Points *, double, double, double, double, int);
int I_get_control_points(char *, struct Control_Points *);
int I_put_control_points(char *, struct Control_Points *);
/* ref.c */
FILE *I_fopen_group_ref_new(char *);
FILE *I_fopen_group_ref_old(char *);
FILE *I_fopen_subgroup_ref_new(char *, char *);
FILE *I_fopen_subgroup_ref_old(char *, char *);
/* sig.c */
int I_init_signatures(struct Signature *, int);
int I_new_signature(struct Signature *);
int I_free_signatures(struct Signature *);
int I_read_one_signature(FILE *, struct Signature *);
int I_read_signatures(FILE *, struct Signature *);
int I_write_signatures(FILE *, struct Signature *);
/* sigfile.c */
FILE *I_fopen_signature_file_new(char *, char *, char *);
FILE *I_fopen_signature_file_old(char *, char *, char *);
/* sigset.c */
int I_SigSetNClasses(struct SigSet *);
struct ClassData *I_AllocClassData(struct SigSet *, struct ClassSig *, int);
int I_InitSigSet(struct SigSet *);
int I_SigSetNBands(struct SigSet *, int);
struct ClassSig *I_NewClassSig(struct SigSet *);
struct SubSig *I_NewSubSig(struct SigSet *, struct ClassSig *);
int I_ReadSigSet(FILE *, struct SigSet *);
int I_SetSigTitle(struct SigSet *, char *);
char *I_GetSigTitle(struct SigSet *);
int I_SetClassTitle(struct ClassSig *, char *);
char *I_GetClassTitle(struct ClassSig *);
int I_WriteSigSet(FILE *, struct SigSet *);
/* sigsetfile.c */
FILE *I_fopen_sigset_file_new(char *, char *, char *);
FILE *I_fopen_sigset_file_old(char *, char *, char *);
/* target.c */
int I_get_target(char *, char *, char *);
int I_put_target(char *, char *, char *);
/* title.c */
int I_get_group_title(char *, char *, int);
int I_put_group_title(char *, char *);
/* var.c */
double I_variance(double, double, int);
double I_stddev(double, double, int);

/* c_assign.c */
int I_cluster_assign(struct Cluster *, int *);
/* c_begin.c */
int I_cluster_begin(struct Cluster *, int);
/* c_clear.c */
int I_cluster_clear(struct Cluster *);
/* c_distinct.c */
int I_cluster_distinct(struct Cluster *, double);
/* c_exec.c */
int I_cluster_exec(struct Cluster *, int, int, double, double, int, int (*)(), int *);
/* c_execmem.c */
int I_cluster_exec_allocate(struct Cluster *);
int I_cluster_exec_free(struct Cluster *);
/* c_means.c */
int I_cluster_means(struct Cluster *);
/* c_merge.c */
int I_cluster_merge(struct Cluster *);
/* c_nclasses.c */
int I_cluster_nclasses(struct Cluster *, int);
/* c_point.c */
int I_cluster_point(struct Cluster *, CELL *);
int I_cluster_begin_point_set(struct Cluster *, int);
int I_cluster_point_part(struct Cluster *, register CELL, int, int);
int I_cluster_end_point_set(struct Cluster *, int);
/* c_reassign.c */
int I_cluster_reassign(struct Cluster *, int *);
/* c_reclass.c */
int I_cluster_reclass(struct Cluster *, int);
/* c_sep.c */
double I_cluster_separation(struct Cluster *, int, int);
/* c_sig.c */
int I_cluster_signatures(struct Cluster *);
/* c_sum2.c */
int I_cluster_sum2(struct Cluster *);

#endif
