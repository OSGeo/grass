#ifndef GRASS_TEMPORAL_H
#define GRASS_TEMPORAL_H

#include <grass/datetime.h>
#include <grass/gis.h>
#include <grass/dbmi.h>


#define TGISDB_DEFAULT_DRIVER "sqlite"
/* Default path in the current location */
#define TGISDB_DEFAULT_SQLITE_PATH "tgis/sqlite.db"


int tgis_set_connection(dbConnection * connection);
int tgis_get_connection(dbConnection * connection);
const char *tgis_get_default_driver_name(void);
char *tgis_get_default_database_name(void);
char *tgis_get_driver_name(void);
char *tgis_get_database_name(void);
char *tgis_get_mapset_driver_name(const char*);
char *tgis_get_mapset_database_name(const char*);
int tgis_set_default_connection(void);


/* ALL CODE BELOW THIS COMMENT
 * IS A PROTOTYPICAL DEFINITION OF THE
 * FUTURE TEMPORAL GIS C-LIBRARY INTERFACE
 * AND NOT YET IMPLEMENTED
 */

#define TGIS_TYPE_MAP          0
#define TGIS_TYPE_STDS         1

#define TGIS_RASTER_MAP   1
#define TGIS_RASTER3D_MAP 2
#define TGIS_VECTOR_MAP   3
#define TGIS_STRDS        4
#define TGIS_STR3DS       5
#define TGIS_STVDS        6

#define TGIS_ABSOLUTE_TIME 0
#define TGIS_RELATIVE_TIME 1

/*! A simple structure to organize time stamped maps*/
typedef struct _tgisMap {
    char *name;
    char *mapset;
    struct TimeStamp ts;
} tgisMap;

/*!
  \brief List of tgisMap struct's
  
  This structure is used to store lists of time stamped maps
  using the tgisMap structure internally.
*/
typedef struct _tgisMapList
{
    /*!
      \brief Array of tgisMap struct's
    */
    tgisMap **values;
    /*!
      \brief Number of tgisMap struct's in the list
    */
    int n_values;
    /*!
      \brief Allocated space for tgisMap struct's
    */
    int alloc_values;
} tgisMapList;

/* map_list.c */
void tgis_init_map_list(tgisMapList *list);
void tgis_free_map_list(tgisMapList *list);
tgisMapList * tgis_new_map_list();
/*! Insert a new map to the map list */
void tgis_map_list_insert(tgisMapList *list, char *name, char*mapset, struct TimeStamp *ts);
/*! Add a new map to the map list */
void tgis_map_list_add(tgisMapList *list, tgisMap *map);

/*!Spatio temporal extent as double values
 
 The spatio temporal extent contains only double values.
 
 The unit of start and end time is seconds in case the time is absolute.
 Reference is Jan. 1. 1900 00:00:00 +00:00 UTC
 
 If no end time is present, because its a time instance, then has_end must be 0.
 
 */
typedef struct _tgisExtent
{
    double start; /*Start time as double value*/
    double end;   /*End time as double value*/
    char has_end; /*Set to 1 if the end time exists, 0 otherwise*/
    double north;
    double south;
    double east;
    double west;
    double top;
    double bottom;
} tgisExtent;

/* Forward declaration */
struct _tgisDataset;

/*!
  \brief List of tgisDatasets struct's

  This structure is used to store lists of dataset (space time datasets or time stamped maps)
  using the tgisDataset structure internally.
*/
typedef struct _tgisDatasetList
{
    /*!
      \brief Array of tgisDataset structs
    */
    struct _tgisDataset **values;
    /*!
      \brief Number of tgisDataset structs in the list
    */
    int n_values;
    /*!
      \brief Allocated space for tgisDataset structs
    */
    int alloc_values;
} tgisDatasetList;

/*! A dataset structure to organize time stamped maps and space time datasets 
    and their spatio-temporal topological relations.
 */
typedef struct _tgisDataset {
    char *name;          /* The name of this dataset */
    char *mapset;        /* The mapset of this dataset */
    char *creator;       /* The creator of this dataset */
    DateTime creation_time; /* The creation time of this dataset */
    char temporal_type;  /* The temporal type of this dataset: TGIS_ABSOLUTE_TIME, 
                            TGIS_RELATIVE_TIME */
    struct TimeStamp ts; /* The timestamp of this dataset */
    tgisExtent extent;   /* This is the spatio-temporal extent represented as double values */

    void *metadata;      /* A pointer to the dataset specific metadata (not used yet) */

    char dataset_type;   /* The type of the dataset: TGIS_RASTER_MAP, TGIS_RASTER3D_MAP,
                            TGIS_VECTOR_MAP, TGIS_STRDS, TGIS_STR3DS, TGIS_STVDS */
    char is_stds;        /* Represent this struct a space time dataset? 1 True, 0 False */

    struct _tgisDataset *next;
    struct _tgisDataset *prev;

    /* Temporal topology relations */
    tgisDatasetList equal;
    tgisDatasetList follows;
    tgisDatasetList precedes;
    tgisDatasetList overlaps;
    tgisDatasetList overlapped;
    tgisDatasetList during;
    tgisDatasetList contains;
    tgisDatasetList starts;
    tgisDatasetList started;
    tgisDatasetList finishes;
    tgisDatasetList finished;

    /* Spatial topology relations */
    tgisDatasetList equivalent;
    tgisDatasetList cover;
    tgisDatasetList covered;
    tgisDatasetList overlap;
    tgisDatasetList in;
    tgisDatasetList contain;
    tgisDatasetList meet;

} tgisDataset;


/* dataset_list.c */
void tgis_init_dataset_list(tgisDatasetList *list);
void tgis_free_dataset_list(tgisDatasetList *list);
tgisDatasetList * tgis_new_dataset_list();
/*! Insert a new dataset to the dataset list */
void tgis_dataset_list_insert(tgisDatasetList *list, char *name, char *mapset, char *creator, 
                           DateTime *creation_time, char temporal_type, 
                           struct TimeStamp *ts, tgisExtent *extent, void *metadata, 
                           char dataset_type, char is_stds);
/*! Add a new dataset to the dataset list */
void tgis_dataset_list_add(tgisDataset *dataset);

/* topology.c */
/*! Build the temporal or spatio-temporal topology of the provided dataset list */
int tgis_build_topology(tgisDatasetList *A, char  spatial);

/*! Build the temporal or spatio-temporal topology between the two dataset list */
int tgis_build_topology2(tgisDatasetList *A, tgisDatasetList *B, char spatial);

/*
 * INTERFACE TO THE TEMPORAL PYTHON FRAMEWORK
 */ 

/* create.c */
/*! Create the new space time dataset */
int tgis_create_stds(char *stds_name, char stds_type, char temporal_type, char *title,
                     char *description, char *semantic_type, char *aggregation_type);

/*! Modify the metadata of an existing space time dataset */
int tgis_modify_stds(char *stds_name, char stds_type, char *title,
                     char *description, char *semantic_type, char *aggregation_type);

/* remove.c */
/*! Remove a space time dataset and optionally all of its maps */
int tgis_remove_stds(char *stds_name, char stds_type, char remove_maps);

/* update.c */
int tgis_update_stds(char *stds_name, char stds_type);

/* register.c */
/*! Register a map in the temporal database and optionally in a space time dataset */
int tgis_register_map(tgisMap *map, char map_type, char *stds_name);

/*! Unregister a map from the temporal database or optionally from a space time dataset */
int tgis_unregister_map(tgisMap *map, char map_type, char *stds_name);

/*! Register maps in the temporal database and optionally in a space time dataset */
int tgis_register_maps(tgisMapList *list, char map_type, char *stds_name);

/*! Unregister maps from the temporal database or optionally from a space time dataset */
int tgis_unregister_maps(tgisMapList *list, char map_type, char *stds_name);

/*! Get all maps that are registered in a space time dataset */
tgisDatasetList *tgis_get_registered_maps(char *stds_name, char *mapset, 
                                          char stds_type, char *order,
                                          char *where);

/* stds.c */
/*! Get all stds from the temporal database */
tgisDatasetList *tgis_get_registered_stds(char *stds_name, char *mapset, 
                                          char stds_type, char temporal_type, 
                                          char *order, char *where);

/*! Get the information about a specific space time dataset from the temporal database */
tgisDataset *tgis_get_stds_info(char *stds_name, char *mapset, char stds_type);

#endif
