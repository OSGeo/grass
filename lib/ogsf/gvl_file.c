#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <grass/gstypes.h>
#include <grass/gsurf.h>
#include <grass/G3d.h>

#define LUCKY                 33

#define MODE_DIRECT            0
#define MODE_SLICE             1
#define MODE_FULL              2
#define MODE_PRELOAD           3

#define MODE_DEFAULT		   0

#define STATUS_READY           0
#define STATUS_BUSY            1

/* structure for slice mode reading from volume file*/
typedef struct
{
    int num, skip;
    int crnt, base;

    void *slice[MAX_VOL_SLICES];
} slice_data;

static geovol_file *Data[MAX_VOL_FILES];
static geovol_file Df[MAX_VOL_FILES];    /* trying to avoid allocation */

static int Numfiles = 0;

static int Cur_id = LUCKY;
static int Cur_max;

static int Rows, Cols, Depths;

/* local functions prototypes */
void *open_g3d_file(char *, IFLAG *, double *, double *);
int close_g3d_file(void *);

/******************************************************************/
/* initialize */
static int init_volfiles(void)
{
    int i;
    G3D_Region *w3;

    for (i = 0; i < MAX_VOL_FILES; i++) {
    /* avoiding dynamic allocation */
    Data[i] = &(Df[i]);
    }

    Cur_max = MAX_VOL_FILES;

    /* get window */
    w3 = GVL_get_window();

    /* set cols, rows, depths from window */
    Cols = w3->cols;
    Rows = w3->rows;
    Depths = w3->depths;

    return (1);
}

/******************************************************************/
/* check number of files */
static int check_num_volfiles(void)
{
    if (Numfiles < Cur_max) {
    return (0);
    }

    fprintf(stderr, "maximum number of datafiles exceeded\n");

    exit(0);

    /* This return statement keeps compilers happy, it is never executed */
    return (0);
}

/******************************************************************/
/* get geovol_file structure for given handle */
geovol_file *gvl_file_get_volfile(int id)
{
    int i;

    for (i = 0; i < Numfiles; i++) {
    if (Data[i]->data_id == id) {
        return (Data[i]);
    }
    }

    return (NULL);
}

/******************************************************************/
/* find file with name and type in geovol_file array an return handle*/
int find_datah(char *name, IFLAG type, int begin)
{
    static int i;
    int start;

    start = begin ? 0 : i + 1;

    for (i = start; i < Numfiles; i++) {
    if (!strcmp(Data[i]->file_name, name)) {
        if (Data[i]->file_type == type) {
            return (Data[i]->data_id);
        }
    }
    }

    return (-1);
}

/******************************************************************/
/* return file name for given handle */
char *gvl_file_get_name(int id)
{
    int i;
    geovol_file *fvf;
    static char retstr[NAME_SIZ];

    for (i = 0; i < Numfiles; i++) {
    if (Data[i]->data_id == id) {
        fvf = Data[i];
        strcpy(retstr, fvf->file_name);

        return (retstr);
    }
    }

    return (NULL);
}

/******************************************************************/
/* return file type for given handle */
int gvl_file_get_file_type(geovol_file *vf)
{
    return (vf->file_type);
}

/******************************************************************/
/* return data type for given handle */
int gvl_file_get_data_type(geovol_file *vf)
{
    return (vf->data_type);
}

/******************************************************************/
/* return minimum and maximum value in volume file */
void gvl_file_get_min_max(geovol_file *vf, double *min, double *max)
{
    *min = vf->min;
    *max = vf->max;
}

/******************************************************************/
/* open volume file */
void *open_volfile(char *name, IFLAG file_type, IFLAG *data_type, double *min, double *max)
{
     if (file_type == VOL_FTYPE_G3D) {
        return open_g3d_file(name, data_type, min, max);
    }

    return (NULL);
}

/******************************************************************/
/* close volume file */
int close_volfile(void *map, IFLAG type)
{
     if (type == VOL_FTYPE_G3D) {
        return close_g3d_file(map);
    }

    return (-1);
}

/******************************************************************/
/* get handle for given file name and type */
int gvl_file_newh(char *name, IFLAG file_type)
{
    geovol_file *new;
    static int first = 1;
    int i, id;
    void *m;
    IFLAG data_type;
    double min, max;

    if (first) {
    if (0 > init_volfiles()) {
        return (-1);
    }

    first = 0;
    }

    if ( 0 <= (id = find_datah(name, file_type, 1))) {
        for (i = 0; i < Numfiles; i++) {
        if (Data[i]->data_id == id) {
            new = Data[i];
            new->count++;

            return (id);
        }
        }
    }

    if (0 > check_num_volfiles()) {
    return (-1);
    }

    if (!name) {
    return (-1);
    }

    if ((m = open_volfile(name, file_type, &data_type, &min, &max)) == NULL) {
    return (-1);
    }

    new = Data[Numfiles];

    if (new) {
    Numfiles++;
    new->data_id = Cur_id++;

    strcpy(new->file_name, name);
    new->file_type = file_type;
    new->count = 1;
    new->map = m;
    new->min = min;
    new->max = max;
    new->data_type = data_type;

    new->status = STATUS_READY;
    new->buff = NULL;

	new->mode = 255;
	gvl_file_set_mode(new, MODE_DEFAULT);

    return (new->data_id);
    }

    return (-1);
}

/******************************************************************/
/* free allocated buffers */
int free_volfile_buffs(geovol_file * vf)
{
    if (vf->mode == MODE_SLICE) {
        free(vf->buff);
        vf->buff = NULL;
    }

	if (vf->mode == MODE_PRELOAD) {
		free(vf->buff);
        vf->buff = NULL;
    }

    return (1);
}

/******************************************************************/
/* free geovol_file structure for given handle */
int gvl_file_free_datah(int id)
{
    int i, j, found = -1;
    geovol_file *fvf;

#ifdef TRACE_FUNCS
    {
    fprintf(stderr, "gvl_file_free_datah\n");
    }
#endif

    for (i = 0; i < Numfiles; i++) {
    if (Data[i]->data_id == id) {
        found = 1;
        fvf = Data[i];

        if (fvf->count > 1) {
            fvf->count--;
        } else {
            close_volfile(fvf->map, fvf->file_type);
            free_volfile_buffs(fvf);
            strcpy(fvf->file_name, "");
            fvf->data_id = 0;

            for (j = i; j < (Numfiles - 1); j++) {
            Data[j] = Data[j + 1];
            }

            Data[j] = fvf;

            --Numfiles;
        }
    }
    }

    return (found);
}

/******************************************************************/
/* reading from G3D raster volume files */
/******************************************************************/

/******************************************************************/
/* open g3d file */
void *open_g3d_file(char *filename, IFLAG *type, double *min, double *max)
{
    char *mapset;
    int itype;
    void *map;

    /* search for g3d file a return his mapset */
    if (NULL == (mapset = G_find_grid3(filename,""))) {
#ifdef DEBUG_MSG
        fprintf(stderr, "can't find grid3 file : %s\n", filename);
#endif
        return (NULL);
    }

    /* open g3d file */
    if (NULL == (map = G3d_openCellOld(filename, mapset, G3D_DEFAULT_WINDOW, G3D_TILE_SAME_AS_FILE, G3D_USE_CACHE_DEFAULT))) {
#ifdef DEBUG_MSG
        fprintf(stderr, "error opening grid3 file : %s\n", filename);
#endif
        return (NULL);
    }

    /* load range into range structure of map */
    if (!G3d_range_load(map)) {
#ifdef DEBUG_MSG
        fprintf(stderr, "error reading range for file : %s\n", filename);
#endif
        return (NULL);
    }

    G3d_range_min_max(map, min, max);

    /* get file data type */
    itype = G3d_fileTypeMap(map);
    if (itype == FCELL_TYPE)
        *type = VOL_DTYPE_FLOAT;
    if (itype == DCELL_TYPE)
        *type = VOL_DTYPE_DOUBLE;

    return (map);
}

/******************************************************************/
/* close g3d file */
int close_g3d_file(void *map)
{
    /* close opened g3d file */
    if (G3d_closeCell(map) != 1) {
#ifdef DEBUG_MSG
        fprintf(stderr, "error closing grid3 file");
#endif
        return (-1);
    }

    return (1);
}

/******************************************************************/
/* read value from g3d file */
int read_g3d_value(IFLAG type, void *map, int x, int y, int z, void *value)
{
    switch (type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            *((float *)value) = G3d_getFloat(map, x, y, z);
            break;

        /* double data type */
        case (VOL_DTYPE_DOUBLE) :
            *((double *)value) = G3d_getDouble(map, x, y, z);
            break;

        /* unsupported data type */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* read slice of values at level from g3d file */
int read_g3d_slice(IFLAG type, void *map, int level, void *data)
{
    int x, y;
    
    switch (type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            for (x = 0; x < Cols; x++) {
                for (y = 0; y < Rows; y++) {       
                    ((float *) data)[x + y * Cols] =
                        G3d_getFloat(map, x, y, level);
                }
            }       
        
            break;
        
        /* double data type */
        case (VOL_DTYPE_DOUBLE) :                        
            for (x = 0; x < Cols; x++) {
                for (y = 0; y < Rows; y++) {       
                    ((double *) data)[x + y * Cols] =
                        G3d_getDouble(map, x, y, level);
                }
            }       
            
            break; 
        
        /* unsupported data type */
        default :
            return (-1);
    }        
        
    return (1);
}

/******************************************************************/
/* read all values from g3d file */
int read_g3d_vol(IFLAG type, void *map, void *data)
{    
    int x, y, z;
    
    switch (type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            for (x = 0; x < Cols; x++) {
                for (y = 0; y < Rows; y++) {
                    for (z = 0; z < Depths; z++) {
                        ((float *) data)[x + y * Cols + z * Rows * Cols] =
                            G3d_getFloat(map, x, y, z);
                    }
                }
            }       
        
            break;
        
        /* double data type */
        case (VOL_DTYPE_DOUBLE) :                        
            for (x = 0; x < Cols; x++) {
                for (y = 0; y < Rows; y++) {
                    for (z = 0; z < Depths; z++) {
                        ((double *) data)[x + y * Cols + z * Rows * Cols] =
                            G3d_getDouble(map, x, y, z);
                    }
                }
            }       
            
            break; 
        
        /* unsupported data type */
        default :
            return (-1);
    }        
        
    return (1);
}

/******************************************************************/
int is_null_g3d_value(IFLAG type, void *value)
{
    switch (type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            return G3d_isNullValueNum(value, FCELL_TYPE);
            break;

        /* double data type */
        case (VOL_DTYPE_DOUBLE) :
            return G3d_isNullValueNum(value, DCELL_TYPE);
            break;

        /* unsupported data type */
        default :
            return (-1);
    }
}

/******************************************************************/
/* reading from buffer */
/******************************************************************/

/******************************************************************/
/* get value from buffer */
int get_buff_value(IFLAG type, void *data, int offset, void *value)
{
    switch (type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            *((float*) value) = ((float *) data)[offset];
            break;

        /* double data type */
        case (VOL_DTYPE_DOUBLE) :
            *((double*) value) = ((double *) data)[offset];
            break;

        /* unsupported data type */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* direct mode reading from volume file */
/******************************************************************/

/******************************************************************/
/* read value direct from volume file */
int get_direct_value(geovol_file *vf, int x, int y, int z, void *value)
{
    switch (vf->file_type) {
        /* G3D file type */
        case (VOL_FTYPE_G3D) :
            if (0 > read_g3d_value(vf->data_type, vf->map, x, y, z, value))
                return (-1);
            break;

        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* full mode reading from volume file */
/******************************************************************/

/******************************************************************/
/* allocate buffer memory for full mode reading */
int alloc_vol_buff(geovol_file *vf)
{
    switch (vf->data_type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            if ((vf->buff = (float*) malloc(sizeof(float) * Cols * Rows * Depths)) == NULL)
                return (-1);
            break;

        /* double data type */
        case (VOL_DTYPE_DOUBLE) :
            if ((vf->buff = (double*) malloc(sizeof(double) * Cols * Rows * Depths)) == NULL)
                return (-1);
            break;

        /* unsupported data type */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* free memory buffer memory */
int free_vol_buff(geovol_file *vf)
{
    free(vf->buff);

    return (1);
}

/******************************************************************/
/* read all values from volume file */
int read_vol(geovol_file *vf)
{
    switch (vf->file_type) {
        /* G3D file format */
        case (VOL_FTYPE_G3D) :
            if (0 > read_g3d_vol(vf->data_type, vf->map, vf->buff))
                    return (-1);
            break;
        /* unsupported file format */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* get value from volume buffer */
int get_vol_value(geovol_file *vf, int x, int y, int z, void *value)
{
    get_buff_value(vf->data_type, vf->buff, z * Rows * Cols + y * Cols + x, value);

    return (1);
}

/******************************************************************/
/* slice mode reading from volume file */
/******************************************************************/

/******************************************************************/
/* allocate buffer for slice mode reading */
int alloc_slice_buff(geovol_file *vf)
{
    int i;
    slice_data *sd = (slice_data *) vf->buff;

    switch (vf->data_type) {
        /* float data type */
        case (VOL_DTYPE_FLOAT) :
            for (i = 0; i < sd->num; i++) {
                if ((sd->slice[i] = (float*) malloc(sizeof(float) * Cols * Rows)) == NULL)
                    return (-1);
            }
            break;

        /* double data type */
        case (VOL_DTYPE_DOUBLE) :
            for (i = 0; i < sd->num; i++) {
                if ((sd->slice[i] = (double*) malloc(sizeof(double) * Cols * Rows)) == NULL)
                    return (-1);
            }
            break;

        /* unsupported data type */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* free buffer for slice mode reading */
int free_slice_buff(geovol_file *vf)
{
    int i;
    slice_data *sd = (slice_data *) vf->buff;

    for (i = 0; i < sd->num; i++) {
        free(sd->slice[i]);
    }

    return (1);
}

/******************************************************************/
/* read slice of values at level from volume file */
int read_slice(geovol_file *vf, int s, int l)
{
    /* get slice structure */
    slice_data *sd = (slice_data *) vf->buff;

    switch (vf->file_type) {
    /* G3D file format */
        case (VOL_FTYPE_G3D) :
            if (0 > read_g3d_slice(vf->data_type, vf->map, l, sd->slice[s]))
                    return (-1);
            break;
        /* unsupported file format */
        default :
            return (-1);
    }

    return (1);
}

/******************************************************************/
/* read new slice into buffer */
void shift_slices(geovol_file *vf)
{
    void *tmp;
    int i;

    slice_data *sd = (slice_data *) vf->buff;

    /* change pointers to slices */
    tmp = sd->slice[0];
    for (i = 0; i < sd->num - 1; i++) {
        sd->slice[i] = sd->slice[i+1];
    }
    sd->slice[sd->num - 1] = tmp;

    /* read new slice data */
    read_slice(vf, sd->num, sd->crnt + 1 + (sd->num - sd->base));

    /* increase current slice value */
    sd->crnt++;
}

/******************************************************************/
/* get value from slice buffer */
int get_slice_value(geovol_file *vf, int x, int y, int z, void *value)
{
    slice_data *sd = (slice_data *) vf->buff;

    /* value is in loaded slices */
    if ((z >= sd->crnt - (sd->base - 1)) &&
        (z <= sd->crnt + sd->num - sd->base)) {

        get_buff_value(vf->data_type, sd->slice[(z - sd->crnt)], y * Cols + x, value);
    }

    /* if value isn't in loaded slices, need read new data slice */
    else if (z == sd->crnt - (sd->base - 1) + 1) {
        shift_slices(vf);
        get_buff_value(vf->data_type, sd->slice[(z - sd->crnt)], y * Cols + x, value);
    }

    /* value out of range */
    else {
        return (-1);
    }

    return (1);
}

/******************************************************************/
/* reading from volume file */
/******************************************************************/

/******************************************************************/
/* start read - allocate memory buffer a read first data into buffer */
int gvl_file_start_read(geovol_file *vf)
{
    int i;
    slice_data *sd;

    /* check status */
    if (vf->status == STATUS_BUSY)
         return (-1);
         
    switch (vf->mode) {
        /* read whole volume into memory */
        case (MODE_FULL) :
             /* allocate memory */
            if (0 > alloc_vol_buff(vf))
                return (-1);

            /* read volume */
            read_vol(vf);
            break;

        /* read first data slices into memory */
        case (MODE_SLICE) :
            /* allocate slices buffer memory */
            if (0 > alloc_slice_buff(vf))
                return (-1);

            /* read volume */
            sd = (slice_data *) vf->buff;
            /* set current slice to 0 */
            sd->crnt = 0;

            /* read first slices into buffer */
            for (i = 0; i < (sd->num - sd->base + 1); i++)
                 read_slice(vf, (sd->base - 1) + i, i);
            break;
     }

     /* set status */
     vf->status = STATUS_BUSY;

    return (1);
}

/******************************************************************/
/* end read - free buffer memory */
int gvl_file_end_read(geovol_file *vf)
{
     /* check status */
     if (vf->status == STATUS_READY)
         return (-1);

    switch (vf->mode) {
        case (MODE_FULL) :
             if (0 > free_vol_buff(vf))
                return (-1);
            break;

        case (MODE_SLICE) :
            /* allocate slices buffer memory */
            if (0 > free_slice_buff(vf))
                return (-1);
     }

     /* set status */
     vf->status = STATUS_READY;

    return (1);
}

/******************************************************************/
/* get value for volume file at x, y, z */
int gvl_file_get_value(geovol_file *vf, int x, int y, int z, void *value)
{
    /*  check status */
    if (vf->status != STATUS_BUSY) {
        return (-1);
    }

    switch (vf->mode) {
        /* read value direct from g3d file */
        case (MODE_DIRECT) :
            if (0 > get_direct_value(vf, x, y, z, value))
                return (-1);
            break;

        case (MODE_SLICE) :
            if (0 > get_slice_value(vf, x, y, z, value))
                return (-1);
            break;

        case (MODE_FULL) :
		case (MODE_PRELOAD) :
            if (0 > get_vol_value(vf, x, y, z, value))
                return (-1);
            break;
    }
    return (1);
}

/******************************************************************/
int gvl_file_is_null_value(geovol_file *vf, void *value)
{
    switch (vf->file_type) {
        /* G3D file format */
        case (VOL_FTYPE_G3D) :
            return is_null_g3d_value(vf->file_type, value);
            break;
        /* unsupported file format */
        default :
            return (-1);
    }
}

/******************************************************************/
/* set parameters for reading volumes */
/******************************************************************/

/******************************************************************/
/* set read mode */
int gvl_file_set_mode(geovol_file *vf, IFLAG mode)
{
    slice_data *sd;

    if (vf->status == STATUS_BUSY)
        return (-1);

    if (vf->mode == mode)
        return (1);

    if (vf->mode == MODE_SLICE)
        free(vf->buff);

	if (vf->mode == MODE_PRELOAD)
        free(vf->buff);


	if (mode == MODE_SLICE) {
        if ((vf->buff = (slice_data *) malloc(sizeof(slice_data))) == NULL)
            return (-1);

        sd = (slice_data *) vf->buff;
        sd->num = 1;
        sd->crnt = 0;
        sd->base = 1;
    }

	if (mode == MODE_PRELOAD) {
		/* allocate memory */
        if (0 > alloc_vol_buff(vf))
          return (-1);

        /* read volume */
        read_vol(vf);
	}

    vf->mode = mode;

    return (1);
}

/******************************************************************/
/* set parameters for slice reading */
int gvl_file_set_slices_param(geovol_file *vf, int n, int b)
{
    slice_data *sd;

    if (vf->status == STATUS_BUSY)
        return (-1);

    if (!(vf->mode == MODE_SLICE))
        return (-1);

    sd = (slice_data *) vf->buff;
    sd->num = n;
    sd->base = b;

    return (1);
}

