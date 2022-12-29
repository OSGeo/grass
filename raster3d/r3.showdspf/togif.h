#define MAXXSIZE 8192
#define HSIZE  5003
typedef int code_int;
typedef long int count_int;
typedef unsigned char char_type;

/*****************************************************************************
 *               vgl_GIFWriter
 * This structure holds all the variables that were GLOBAL and non-constant
 * in the original Haeberli TOGIF source.
 *****************************************************************************
 */
typedef struct _vgl_GIFWriter
{
    unsigned long *buffer;	/* standard 32 bit pixel buffer */
    unsigned short xsize;	/* width of image */
    unsigned short ysize;	/* height of image */
    int iscolor;		/* color or bw flag */

	       /************** getgifpix2() variables ************************/
    int currow;
    short rbuf[MAXXSIZE];
    short gbuf[MAXXSIZE];
    short bbuf[MAXXSIZE];
    short obuf[MAXXSIZE];

	       /************** GIF specific variables *****************/
    unsigned short curx, cury;
    long countDown;
    int pass;
    int interlace;
    unsigned long cur_accum;
    int cur_bits;

		/************ GIF image compression variables **************/
    int n_bits;			/* number of bits/code */
    int maxbits;		/* user settable max # bits/code */
    code_int maxcode;		/* maximum code, given n_bits */
    code_int maxmaxcode;	/* should NEVER generate this code */
    count_int htab[HSIZE];
    unsigned short codetab[HSIZE];
    code_int hsize;		/* for dynamic table sizing */
    code_int free_ent;		/* first unused entry */
    int clear_flg;
    int offset;
    long int in_count;		/* length of input */
    long int out_count;		/* # of codes output (for debugging) */
    int g_init_bits;
    int clearCode;
    int EOFCode;
    FILE *g_outfile;
    int a_count;
    char accum[256];

	       /************** gammawarp() variables ************************/
    float curgamma;
    short gamtab[256];

	       /************** ditherrow() variables ************************/
    short **rtab;
    short **gtab;
    short **btab;

} vgl_GIFWriter;
vgl_GIFWriter *vgl_GIFWriterBegin(void);
void vgl_GIFWriterWriteGIFFile(vgl_GIFWriter * gifwriter,
			       unsigned long *buffer, int xsize, int ysize,
			       int bwflag, FILE * outf);
void vgl_GIFWriterEnd(vgl_GIFWriter * gifwriter);
