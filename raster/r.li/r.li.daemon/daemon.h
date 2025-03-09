/**
 * \file daemon.h
 *
 * \brief Types and function of r.li raster analysis
 * server
 *
 *
 * \author Claudio Porta & Lucio Davide Spano
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \version 1.0
 *
 * \include stdlib.h
 *
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include "list.h"
#include "defs.h"

#define NORMAL 1
#define MVWIN  2
#define GEN    3

/**
 * \brief descriptor of a worker
 * \member pid worker process identifier
 * \member pipe name of pipe to receive message
 */
typedef struct wd {
    int pid;
    char *pipe;
    int channel;
} wd;

/**
 * \brief entry of cell memory menager
 * \member used number of rows in cache
 * \member cache cache matrix
 * \member contents line numbers of elements in cache
 */
struct cell_memory_entry {
    int used;
    CELL **cache;
    int *contents;
};

/**
 * \brief cell memory menager definition
 */
typedef struct cell_memory_entry *cell_manager;

/**
 * \brief entry of dcell memory menager
 * \member used number of rows in cache
 * \member cache cache matrix
 * \member contents line numbers of elements in cache
 */
struct dcell_memory_entry {
    int used;
    DCELL **cache;
    int *contents;
};

/**
 * \brief dcell memory menager definition
 */
typedef struct dcell_memory_entry *dcell_manager;

/**
 * \brief entry of fcell memory menager
 * \member used number of rows in cache
 * \member cache cache matrix
 * \member contents line numbers of elements in cache
 */
struct fcell_memory_entry {
    int used;
    FCELL **cache;
    int *contents;
};

/**
 * \brief dcell memory menager definition
 */
typedef struct fcell_memory_entry *fcell_manager;

/**
 * \brief fields of an area descriptor
 * \member x column offset = start of sample area
 * \member y row offset = start of sample area
 * \member rl sample area length in rows
 * \member cl sample area length in columns
 * \member rc number of rows in the cache
 * \member mask file descriptor of mask raster file (-1 if there is no mask)
 */
struct area_entry {
    int x;
    int y;
    int rl;
    int cl;
    int rc;
    int mask;
    int data_type;
    cell_manager cm;
    dcell_manager dm;
    fcell_manager fm;
    char *raster;
    char *mask_name;
};

/**
 * \brief function prototype for index calculation
 * \param fd file descriptor of opened raster map
 * \param par optional parameters
 * \param ad definition of the sample area
 * \param result pointer to store the result
 * \return RLI_ERRORE error occurs in calculating index
 * \return RLI_OK  otherwise
 */
typedef int rli_func(int fd, char **par, struct area_entry *ad, double *result);

/**
 * \brief applies the f index once for every
 * area defined in setup file
 * \param file name of setup file
 * \param f the function that defines the index
 * \param raster the raster file to analyze
 * \return 1 error occurs in calculating index
 * \return 0 otherwise
 *
 * \note
 * Unlike other function in r.li, this function returns return code
 * usable as process return code rather than using true/false (1/0)
 * idiom for success/failure. The interface was designed to accommodate
 * common usage of this function in r.li modules.
 */

int calculateIndex(char *file, rli_func *f, char **parameters, char *raster,
                   char *output);

/**
 * \description parses the setup file and populates the list of areas
 * to analyze
 * \param setup the setup file
 * \param list l the list of areas to analyze
 * \param g areas generator for moving window analysis
 * \param raster raster file to analyze
 * \return NORMAL if the output had to be written in normal way and
 * list had to be used
 * \return GEN if the generator had to be used and the output had to
 * be written in normal way
 * \return MVWIN if a new raster file had to be created
 */
int parseSetup(char *path, struct list *l, struct g_area *g, char *raster);

/**
 * \description dispose sample areas if configuration file have
 *  runtime disposition
 * \param l the list where to insert the areas
 * \param g the area generator to initialize
 * \param def the setup file line with the definition of disposition
 * \return NORMAL if the output had to be written in normal way and
 * list had to be used
 * \return GEN if the generator had to be used and the output had to
 * be written in normal way
 * \return MVWIN if a new raster file had to be created
 */
int disposeAreas(struct list *l, struct g_area *g, char *def);

/**
 * \brief generate the next area to analyze
 * \param parsed the output of a previous parseSetup function call
 * \param g the area generator
 * \param l the list of area
 * \param m the next message
 * \return 1 if the area is generated
 * \return 0 if there isn't another area
 */
int next_Area(int parsed, struct list *l, struct g_area *g, msg *m);

/**
 * \brief writes output in a file
 * \param out the output file
 * \param m the done message receive from a worker
 * \return 1 success
 * \return 0 fail
 */
int print_Output(int out, msg m);

/**
 * \brief writes errors in a file
 * \param out the output file
 * \param m the error message receive from a worker
 * \return 1 success
 * \return 0 fail
 */
int error_Output(int out, msg m);

/**
 * \brief client implementation
 * \param raster the raster map to analyze
 * \param f the function used for index computing
 * \param result where to put the result of index computing
 */
void worker_init(char *raster, rli_func *f, char **parameters);
void worker_process(msg *ret, msg *m);
void worker_end(void);

/**
 * \brief adapts the mask at current raster file
 * \param mask name of mask raster file
 * \param raster the name of current raster file
 * \param rl the length in rows of sample area
 * \param cl the length in cols of sample area
 * \return the name of mask raster file to use
 */
char *mask_preprocessing(char *mask, char *raster, struct area_entry *ad);

/**
 * \brief writes the output for a raster file
 *        \param fd file descriptor for writing
 * \param aid the area identifier of result
 * \param res the result to be written
 * \return 0 on error, 1 if done
 */
int raster_Output(int fd, int aid, struct g_area *g, double res);

/**
 * \brief calculates a simple index for code debugging
 * \param fd file descriptor of raster
 * \param par the parameters of index not included in function
 * declaration
 * \param result where to return result
 * \return 0 on error, 1 otherwise
 */
int simple_index(int fd, char **par, struct area_entry *ad, double *result);

/**
 * \brief copy the content of regular file random access
 * on raster mv_fd
 * \param mv_fd the raster to write
 * \param random_access the regular file
 * \param g the mv window generator
 * \return 0 on error, 1 otherwise
 */
int write_raster(int mv_fd, int random_access, struct g_area *g);

/**
 * \brief get a cell raster row using the memory menager
 * \param fd file descriptor of raster to analyze
 * \param row identifier of row to get
 * \param ad area descriptor of current sample area
 */
CELL *RLI_get_cell_raster_row(int fd, int row, struct area_entry *ad);

/**
 * \brief get a dcell raster row using the memory menager
 * \param fd file descriptor of raster to analyze
 * \param row identifier of row to get
 * \param ad area descriptor of current sample area
 */
DCELL *RLI_get_dcell_raster_row(int fd, int row, struct area_entry *ad);

/**
 * \brief get a fcell raster row using the memory menager
 * \param fd file descriptor of raster to analyze
 * \param row identifier of row to get
 * \param ad area descriptor of current sample area
 */
FCELL *RLI_get_fcell_raster_row(int fd, int row, struct area_entry *ad);
