#include <string.h>
#include <grass/gis.h>
#include "viz.h"


/*================= DOCUMENT RETURN VALUES! =================*/

int dfwrite_header(file_info * headp)
{
    int isize, flsize;
    cmndln_info *linep;
    FILE *fp;
    long Where_dataoff;
    long Where_lookoff;

    linep = &(headp->linefax);
    fp = headp->dspfoutfp;

    isize = sizeof(int);
    flsize = sizeof(float);
    /* print the header code on first line of file */
    if (!fwrite(DSPF_ID, strlen(DSPF_ID), 1, fp))
	return (-1);
    /* the dimensions of the data */
    if (1 != fwrite(&headp->xdim, isize, 1, fp))
	return (-1);
    if (1 != fwrite(&headp->ydim, isize, 1, fp))
	return (-1);
    if (1 != fwrite(&headp->zdim, isize, 1, fp))
	return (-1);


    /* print out code for min and max values */
    if (1 != fwrite(&headp->min, flsize, 1, fp))
	return (-1);
    if (1 != fwrite(&headp->max, flsize, 1, fp))
	return (-1);

    /* the litmodel stored for each polygon */
    if (1 != fwrite(&linep->litmodel, isize, 1, fp))
	return (-1);

    /* write the total number of thresholds to be searched for */
    if (1 != fwrite(&linep->nthres, isize, 1, fp))
	return (-1);
    /* write the array of thresholds out */
    if ((fwrite(linep->tvalue, flsize, linep->nthres, fp)) != linep->nthres) {
	fprintf(stderr, "ERROR: fwrite in dspf_header.c\n");
	return (-1);
    }

    /* write the offset to the lookup table */
    /* the first time this number is set to 0 */
    /*this information will be overwritten after dspf is done */
    /* G_ftell keeps track of where this information is to be placed */
    Where_lookoff = G_ftell(fp);
    headp->Lookoff = 0;
    if (1 != fwrite(&headp->Lookoff, sizeof(long), 1, fp))
	return (-1);

    /* code to determine the length of the binary file header */
    /* Dataoff = length of the header */
    /*Dataoff = strlen (DSPF_ID) + 7*isize + 5*flsize + linep->nthres*flsize; */
    Where_dataoff = G_ftell(fp);
    headp->Dataoff = 0;
    if (1 != fwrite(&headp->Dataoff, sizeof(long), 1, fp))
	return (-1);



    /* End of header,  now go back and fill in what we can */
    headp->Dataoff = G_ftell(fp);
    G_fseek(fp, Where_dataoff, 0);
    if (1 != fwrite(&headp->Dataoff, sizeof(long), 1, fp))
	return (-1);

    G_fseek(fp, headp->Dataoff, 0);	/* and return to begin writing data */

    /* will still have to come back once more to fill in Lookup offset */

    return (0);
}


/**************************** dfread_header **********************************/

/**************************** dfread_header **********************************/

/**************************** dfread_header **********************************/

int dfread_header(file_info * headp)
{
    int isize, flsize;
    FILE *fp;
    cmndln_info *linep;
    char buf[80];
    int len;

    fp = headp->dspfinfp;


    len = strlen(DSPF_ID);
    G_fseek(fp, 0L, 0);		/* rewind file */
    /*read in header information and store in File_info struct */

    if (!fread(buf, 1, len, fp))
	return (-1);
    buf[len] = 0;
    if (strncmp(DSPF_ID, buf, len)) {
	if (!strncmp("dspf003.01", buf, len))
	    return (dfread_header_old(headp, fp));

	fprintf(stderr, "Error: header mismatch '%s' - '%s'\n", DSPF_ID, buf);
	return (-1);
    }
    linep = &(headp->linefax);
    isize = sizeof(int);
    flsize = sizeof(float);

    if (!fread(&headp->xdim, isize, 1, fp))
	return (-1);
    if (!fread(&headp->ydim, isize, 1, fp))
	return (-1);
    if (!fread(&headp->zdim, isize, 1, fp))
	return (-1);
    if (!fread(&headp->min, flsize, 1, fp))
	return (-1);
    if (!fread(&headp->max, flsize, 1, fp))
	return (-1);
    if (!fread(&linep->litmodel, isize, 1, fp))
	return (-1);
    if (!fread(&linep->nthres, isize, 1, fp))
	return (-1);
    if (!fread(linep->tvalue, flsize, linep->nthres, fp))
	return (-1);
    if (!fread(&headp->Lookoff, isize, 1, fp))
	return (-1);
    if (!fread(&headp->Dataoff, isize, 1, fp))
	return (-1);

    print_head_info(headp);

    return (1);
}

int dfread_header_old(file_info * headp, FILE * fp)
{
    int isize, flsize;
    cmndln_info *linep;
    float tmp;

    linep = &(headp->linefax);
    isize = sizeof(int);
    flsize = sizeof(float);

    if (!fread(&headp->xdim, isize, 1, fp))
	return (-1);
    if (!fread(&headp->ydim, isize, 1, fp))
	return (-1);
    if (!fread(&headp->zdim, isize, 1, fp))
	return (-1);
    if (!fread(&tmp, flsize, 1, fp))
	return (-1);
    if (!fread(&tmp, flsize, 1, fp))
	return (-1);
    if (!fread(&tmp, flsize, 1, fp))
	return (-1);
    if (!fread(&headp->min, flsize, 1, fp))
	return (-1);
    if (!fread(&headp->max, flsize, 1, fp))
	return (-1);
    if (!fread(&linep->litmodel, isize, 1, fp))
	return (-1);
    if (!fread(&linep->nthres, isize, 1, fp))
	return (-1);
    if (!fread(linep->tvalue, flsize, linep->nthres, fp))
	return (-1);
    if (!fread(&headp->Lookoff, isize, 1, fp))
	return (-1);
    if (!fread(&headp->Dataoff, isize, 1, fp))
	return (-1);

    print_head_info(headp);

    return (1);
}
