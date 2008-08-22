#define TRUE 1
#define FALSE 0

#define MAXEDLINES  50
#define RECORD_LEN  80
#define NEWLINE     '\n'
#define RECLASS_TABLE 1
#define RECLASS_RULES 2
#define RECLASS_SCALE 3

#define METERS    1
#define FEET      2
#define DEGREES   3

#define CELL_TYPE 0
#define FCELL_TYPE 1
#define DCELL_TYPE 2

#define PROJECTION_XY  0
#define PROJECTION_UTM 1
#define PROJECTION_SP  2
#define PROJECTION_LL  3
#define PROJECTION_OTHER  99

#define PROJECTION_FILE "PROJ_INFO"
#define UNIT_FILE "PROJ_UNITS"

#define G_VAR_GISRC    0
#define G_VAR_MAPSET   1

#define G_GISRC_MODE_FILE     0   
#define G_GISRC_MODE_MEMORY   1   

#define TYPE_INTEGER  1
#define TYPE_DOUBLE   2
#define TYPE_STRING   3
#define YES           1
#define NO            0

typedef enum {
    G_OPT_DB_WHERE,   
    G_OPT_R_INPUT, 
    G_OPT_R_OUTPUT,
    G_OPT_R_MAP,   
    G_OPT_V_INPUT, 
    G_OPT_V_OUTPUT,
    G_OPT_V_MAP,   
    G_OPT_V_TYPE,  
    G_OPT_V_FIELD, 
    G_OPT_V_CAT,   
    G_OPT_V_CATS   
} STD_OPT;

#define G_INFO_FORMAT_STANDARD 0  
#define G_INFO_FORMAT_GUI      1  

#define G_ICON_CROSS  0
#define G_ICON_BOX    1
#define G_ICON_ARROW  2

#define DEFAULT_FG_COLOR "black"
#define DEFAULT_BG_COLOR "white"


typedef int CELL;
typedef double DCELL;
typedef float FCELL;
/*extern CELL CELL_NODATA; Sajith */

typedef int RASTER_MAP_TYPE;

struct Cell_head
{
    int format;     
    int compressed; 
    int rows;	    
    int rows3;	    
    int cols;	    
    int cols3;	    
    int depths;     
    int proj;	    
    int zone;	    
    double ew_res;  
    double ew_res3; 
    double ns_res;  
    double ns_res3; 
    double tb_res;  
    double north;   
    double south;
    double east;
    double west;
    double top;
    double bottom;
};

struct _Color_Rule_
{
    struct
    {
    	DCELL value;
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
    }  high;
    struct
    {
    	DCELL value;
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
    }  low;

/*    struct _Color_Rule_ *next;
    struct _Color_Rule_ *prev;  Commented By sajith I am confued here....*/
};

struct _Color_Info_
{
    struct _Color_Rule_ *rules;
    int n_rules;

    struct
    {
    	unsigned char *red;
	unsigned char *grn;
	unsigned char *blu;
	unsigned char *set;
	int nalloc;
	int active;
    } lookup;

    struct
    {
        DCELL *vals;
	struct _Color_Rule_ **rules;
	int nalloc;
	int active;
    } fp_lookup;

    DCELL min, max;
};

struct Colors
{
    int version;	
    DCELL shift;
    int invert;
    int is_float;   	    
    int null_set;   	    
    unsigned char null_red;
    unsigned char null_grn;
    unsigned char null_blu;
    int undef_set;  	   
    unsigned char undef_red;
    unsigned char undef_grn;
    unsigned char undef_blu;
    struct _Color_Info_ fixed;
    struct _Color_Info_ modular;
    DCELL cmin;
    DCELL cmax;
};

struct Reclass
{
    char name[50];  	   
    char mapset[50]; 	   
    int type;	    	   
    int num;	    	   
    CELL min;	    	   
    CELL max;	    	   
    CELL *table;    	   
} ;

struct FPReclass_table
{
    DCELL dLow;     
    DCELL dHigh;    
    DCELL rLow;     
    DCELL rHigh;    
};

struct FPReclass
{
    int defaultDRuleSet;   
    int defaultRRuleSet;   
    int infiniteLeftSet;   
    int infiniteRightSet;  
    int rRangeSet;  	   
    int maxNofRules; 
    int nofRules;
    DCELL defaultDMin;     
    DCELL defaultDMax;     
    DCELL defaultRMin;     
    DCELL defaultRMax;     
    DCELL infiniteDLeft;   
    DCELL infiniteDRight;  
    DCELL infiniteRLeft;   
    DCELL infiniteRRight;  
    DCELL dMin;     	   
    DCELL dMax;     	   
    DCELL rMin;     	   
    DCELL rMax;     	   
    struct FPReclass_table *table;
};

struct Quant_table
{
    DCELL dLow;
    DCELL dHigh;
    CELL cLow;
    CELL cHigh;
};

struct Quant
{
    int truncate_only;
    int round_only;
    int defaultDRuleSet;
    int defaultCRuleSet;
    int infiniteLeftSet;
    int infiniteRightSet;
    int cRangeSet;
    int maxNofRules;
    int nofRules;
    DCELL defaultDMin;
    DCELL defaultDMax;
    CELL defaultCMin;
    CELL defaultCMax;
    DCELL infiniteDLeft;
    DCELL infiniteDRight;
    CELL infiniteCLeft;
    CELL infiniteCRight;
    DCELL dMin;
    DCELL dMax;
    CELL cMin;
    CELL cMax;
    struct Quant_table *table;

    struct
    {
	DCELL *vals;

	struct Quant_table **rules;
	int nalloc;
	int active;
	DCELL inf_dmin; 
	DCELL inf_dmax; 
	CELL inf_min; 
	CELL inf_max; 
    } fp_lookup;
};

struct Categories
{
    CELL ncats;     	    
    CELL num;	    	    
    char *title;    	    
    char *fmt;	    	    
    float m1;	    	    
    float a1;	    	    
    float m2;	    	    
    float a2;	    	    
    struct Quant q; 	    
    char **labels;  	    
    int *marks;    	    
    int nalloc;
    int last_marked_rule;
   
};

struct History
{
    char    mapid[RECORD_LEN];
    char    title[RECORD_LEN];
    char    mapset[RECORD_LEN];
    char    creator[RECORD_LEN];
    char    maptype[RECORD_LEN];
    char    datsrc_1[RECORD_LEN];
    char    datsrc_2[RECORD_LEN];
    char    keywrd[RECORD_LEN];
    int     edlinecnt;
    char    edhist[MAXEDLINES][RECORD_LEN];
};

struct Cell_stats
{
    struct Cell_stats_node
    {
	int idx;
	long *count;
	int left;
	int right;
    } *node ;    
 
    int tlen ;   
    int N;       
    int curp;
    long null_data_count;   
    int curoffset;
};

struct Histogram
{
    int num;
    
    struct Histogram_list
    {
	CELL cat;
	long count;
    } *list;
};

struct Range
{
    CELL min;		
    CELL max;	
    int first_time;   
};

struct FPRange
{
    DCELL min;		
    DCELL max;	
    int first_time;  
};

struct G_3dview
{
    char pgm_id[40];        
    float from_to[2][3];    
    float fov;              
    float twist;            
    float exag;             
    int mesh_freq;  	    
    int poly_freq;   	    
    int display_type;       
    int lightson;   	    
    int dozero;   	    
    int colorgrid;   	    
    int shading;   	    
    int fringe;   	    
    int surfonly;   	    
    int doavg;   	    
    char grid_col[40];	    
    char bg_col[40];	    
    char other_col[40];     
    float lightpos[4];	    
    float lightcol[3];      
    float ambient;
    float shine;
    struct Cell_head vwin;
};

struct Key_Value
{
    int nitems;
    int nalloc;
    char **key;
    char **value;
};

struct Option                	   
{
    char *key;                    
    int type;                     
    int required;                 
    int multiple;                 
    char *options;                
    char **opts;                  
    char *key_desc;               
    char *label;                  
    char *description;            
    char *descriptions;           
    char **descs;                 
    char *answer;                 
    char *def;                    
    char **answers;               
    struct Option *next_opt;      
    char *gisprompt;              
    int (*checker)();             
    int count;
};

struct Flag                 	   
{
    char key;                      
    char answer;                   
    char *label;                   
    char *description;             
    struct Flag *next_flag;        
};

struct GModule                     
{
    char *label;                   
    char *description;             
    int overwrite;
};

struct TimeStamp
{
    DateTime dt[2];   
    int count;
};

