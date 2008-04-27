#define INAME_LEN 30
struct Ref_Files
{
    char name[30]; 
    char mapset[30];
};
struct Ref_Color
{
    unsigned char *table      ;  
    unsigned char *index      ;  
    unsigned char *buf        ;  
    int fd                    ;  
    CELL min, max             ;  
    int n                     ;  
};

struct Ref
{
    int nfiles;
    struct Ref_Files *file;
    struct Ref_Color red, grn, blu;
} ;

struct Tape_Info
{
    char title[75];
    char id[2][75];
    char desc[5][75];
} ;

struct Control_Points
{
    int  count;
    double *e1;
    double *n1;
    double *e2;
    double *n2;
    int *status;
} ;
    struct One_Sig
    {
	char desc[100];
	int npoints;
	double *mean;	
	double **var;   
	int status;     
	float r,g,b;	
	int have_color;
    }; 
struct Signature
{
    int nbands;
    int nsigs;
    char title[100];
    struct One_Sig *sig;

} ;

struct Cluster
{
    int nbands;
    int npoints;
    CELL **points ;
    int np;

    double *band_sum     ; 
    double *band_sum2    ; 

    int    *class        ; 
    int    *reclass      ; 
    int    *count        ; 
    int    *countdiff    ; 
    double **sum         ; 
    double **sumdiff     ; 
    double **sum2        ; 
    double **mean        ; 
    struct Signature S   ; 

    int nclasses;
    int merge1, merge2;
    int iteration;
    double percent_stable;
} ;
    struct SubSig
        {
            double N;
	    double pi;
            double *means;
            double **R;
            double **Rinv;
	    double cnst;
            int used;
        } ;
        struct ClassData
        {
	    int npixels;
	    int count;
	    double **x;   
	    double **p;   
        };

    struct ClassSig
    {
	long classnum;
        char *title;
        int used;
	int type;
        int nsubclasses;
    struct SubSig *SubSig;
        struct ClassData ClassData;
    };

struct SigSet
{
    int nbands;
    int nclasses;
    char *title;
    struct ClassSig *ClassSig;
};

#define SIGNATURE_TYPE_MIXED 1

#define GROUPFILE "CURGROUP"
#define SUBGROUPFILE "CURSUBGROUP";
