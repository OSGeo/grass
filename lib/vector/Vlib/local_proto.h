#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <grass/vector.h>

/*! Cache type (see Format_info_cache) */
#define CACHE_FEATURE          0
#define CACHE_MAP              1

/*! Attributes of temporary maps */
/* #define TEMPORARY_MAP_DB */

/*! Temporary mode */
#define TEMPORARY_MAP_DISABLED 0
#define TEMPORARY_MAP_ENV      1
#define TEMPORARY_MAP          2

/* Internal vector library subroutines which are not part of public
   API */

/* area.c */
<<<<<<< HEAD
<<<<<<< HEAD
int Vect__get_area_points(struct Map_info *, const plus_t *, int,
                          struct line_pnts *);
int Vect__get_area_points_nat(struct Map_info *, const plus_t *, int,
=======
int Vect__get_area_points(const struct Map_info *, const plus_t *, int,
                          struct line_pnts *);
int Vect__get_area_points_nat(const struct Map_info *, const plus_t *, int,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
int Vect__get_area_points(const struct Map_info *, const plus_t *, int,
                          struct line_pnts *);
int Vect__get_area_points_nat(const struct Map_info *, const plus_t *, int,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                              struct line_pnts *);

/* close.c */
void Vect__free_cache(struct Format_info_cache *);
void Vect__free_offset(struct Format_info_offset *);

/* copy.c */
int Vect__copy_areas(struct Map_info *, int, struct Map_info *);

/* map.c */
int Vect__delete(const char *, int);

/* open.c */
int Vect__open_old(struct Map_info *, const char *, const char *, const char *,
                   int, int, int);
<<<<<<< HEAD
<<<<<<< HEAD
char *Vect__get_path(char *, struct Map_info *);
char *Vect__get_element_path(char *, struct Map_info *, const char *);
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
char *Vect__get_path(char *, const struct Map_info *);
char *Vect__get_element_path(char *, const struct Map_info *, const char *);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

/* write_nat.c */
int V2__add_line_to_topo_nat(struct Map_info *, off_t, int,
                             const struct line_pnts *, const struct line_cats *,
                             int,
<<<<<<< HEAD
<<<<<<< HEAD
                             int (*external_routine)(struct Map_info *, int));
=======
                             int (*external_routine)(const struct Map_info *,
                                                     int));
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                             int (*external_routine)(const struct Map_info *,
                                                     int));
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
int V2__delete_line_from_topo_nat(struct Map_info *, int, int,
                                  const struct line_pnts *,
                                  const struct line_cats *);

/* write_sfa.c */
off_t V2__write_area_sfa(struct Map_info *, const struct line_pnts **, int,
                         const struct line_cats *);

/* write_ogr.c */
#ifdef HAVE_OGR
off_t V2__write_area_ogr(struct Map_info *, const struct line_pnts **, int,
                         const struct line_cats *);
#endif /* HAVE_OGR */

#endif /* PG_LOCAL_PROTO_H__ */
