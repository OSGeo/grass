/****************************************************************************
 * 
 *  MODULE:	r.terraflow
 *
 *  COPYRIGHT (C) 2007 Laura Toma
 *   
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/


#ifndef _gras2str_H
#define _gras2str_H

#include <grass/iostream/ami.h>
#include "option.h"
#include "types.h"
#include "common.h"
#include "nodata.h" /* for TERRAFLOW_INTERNAL_NODATA_VALUE */





/* ---------------------------------------------------------------------- */
/* create and return a STREAM containing the given cell; nodata_count
   is set to the number of cells that contain nodata */
template<class T>
AMI_STREAM<T>*
cell2stream(char* cellname, elevation_type T_max_value, long* nodata_count) {
  Rtimer rt;
  AMI_err ae; 
  elevation_type T_min_value = -T_max_value;
  
  rt_start(rt); 
  assert(cellname && nodata_count);
  *nodata_count = 0;
  /* create output stream */
  AMI_STREAM<T>* str = new AMI_STREAM<T>();
  { 
    char * foo;
    str->name(&foo); 
    *stats << "reading raster map " << cellname 
		   << "to stream " << foo << endl;
    if (opt->verbose) 
      fprintf(stderr, "reading data from %s to stream %s: ", cellname, foo);
  }
  
  char *mapset;
  mapset = G_find_cell (cellname, "");
  if (mapset == NULL)
    G_fatal_error (_("Raster map <%s> not found"), cellname);
  
  /* open map */
  int infd;
  if ( (infd = G_open_cell_old (cellname, mapset)) < 0)
    G_fatal_error (_("Unable to open raster map <%s>"), cellname);
  
  /* determine map type (CELL/FCELL/DCELL) */
  RASTER_MAP_TYPE data_type;
  data_type = G_raster_map_type(cellname, mapset);
  
  /* Allocate input buffer */
  void *inrast;
  inrast = G_allocate_raster_buf(data_type);

  CELL c;
  FCELL f;
  DCELL d;
  T x;
  int isnull = 0;

  for (int i = 0; i< nrows; i++) {
	
	/* read input map */
    if (G_get_raster_row (infd, inrast, i, data_type) < 0)
      G_fatal_error (_("Unable to read raster map <%s>, row %d"),cellname, i);
  
	for (int j=0; j<ncols; j++) {

	  switch (data_type) {
      case CELL_TYPE:
		c = ((CELL *) inrast)[j];
		isnull = G_is_c_null_value(&c);
		if (!isnull) {
		  x = (T)c; d = (DCELL)c;
		}
		break;
      case FCELL_TYPE:
		f = ((FCELL *) inrast)[j];
		isnull = G_is_f_null_value(&f);
		if (!isnull) {
		  x = (T)f; d = (DCELL)f;
		}
		break;
      case DCELL_TYPE:
		d = ((DCELL *) inrast)[j];
		isnull = G_is_d_null_value(&d);
		if (!isnull) {		
		  x = (T)d;
		}
		break;
	  default:
		G_fatal_error("Raster type not implemented");		
      }
	  /* cout << form("(i=%d,j=%d): (%d, %f)\n",i,j,x,d); cout.flush(); */
	  /* handle null values */
	  if (isnull) {
	    x = TERRAFLOW_INTERNAL_NODATA_VALUE;
		(*nodata_count)++;
	  } else {
		/* check range */
		if ((d > (DCELL)T_max_value) || (d < (DCELL)T_min_value)) {
		  fprintf(stderr, "reading raster map %s at (i=%d,j=%d) value=%.1f\n",
				  cellname, i, j, d);
		  G_fatal_error("value out of range.");
		}
	  }  
	  /* write x to stream */
	  ae = str->write_item(x);
	  assert(ae == AMI_ERROR_NO_ERROR); 
	  
	} /* for j */

	if (opt->verbose) G_percent(i, nrows, 2);
  }/* for i */

  if (opt->verbose)
	G_percent(1, 1, 2); /* finish it */

  /* delete buffers */
  G_free(inrast);
  /* close map files */
  G_close_cell (infd);
  
  assert(nrows * ncols == str->stream_len());
  rt_stop(rt);
  stats->recordTime("reading raster map", rt);

  return str;
}





/* ---------------------------------------------------------------------- */
template<class T>
void 
stream2_CELL(AMI_STREAM<T>* str, dimension_type nrows, dimension_type ncols, 
	    char* cellname, bool usefcell=false) {
  Rtimer rt;
  AMI_err ae; 
  RASTER_MAP_TYPE mtype= (usefcell ? FCELL_TYPE : CELL_TYPE);
  
  rt_start(rt); 
  assert(str);
  assert(str->stream_len() == nrows*ncols);
  str->seek(0);
  {
    char * foo;
    str->name(&foo); 
    *stats << "writing stream " << foo << " to raster map  " << cellname << "\n";
    fprintf(stderr, "writing stream %s to raster map %s: ", foo, cellname);
  }

  /* open output raster map */
  int outfd;
  if ( (outfd = G_open_raster_new (cellname, mtype)) < 0) {
    G_fatal_error (_("Unable to create raster map <%s>"), cellname);
  }
  
  /* Allocate output buffer */
  unsigned char *outrast;
  outrast = (unsigned char *)G_allocate_raster_buf(mtype);
  assert(outrast);
 
  T* elt;
  for (int i=0; i< nrows; i++) {
	for (int j=0; j< ncols; j++) {
	  
	  /* READ VALUE */
	  ae = str->read_item(&elt);
	  if (ae != AMI_ERROR_NO_ERROR) {
		str->sprint();
		fprintf(stderr, "reading stream failed at (%d,%d)\n", i,j);
		G_fatal_error("stream2cell failed");
	  }

	  /* WRITE VALUE */
    if(usefcell){
      if (is_nodata(*elt)) {
        G_set_f_null_value( &( ((FCELL *) outrast)[j]), 1);
      } else { 
        ((FCELL *) outrast)[j] = (FCELL)(*elt);
      }
    }else{
      if (is_nodata(*elt)) {
        G_set_c_null_value( &( ((CELL *) outrast)[j]), 1);
      } else { 
        ((CELL *) outrast)[j] = (CELL)(*elt);
      }
    }

  } /* for j*/
  if (G_put_raster_row (outfd, outrast, mtype) < 0)
    G_fatal_error ("Cannot write to <%s>",cellname);

  G_percent(i, nrows, 2);
  }/* for i */
  G_percent(1, 1, 2); /* finish it */

  G_free(outrast);
  G_close_cell (outfd);

  rt_stop(rt);
  stats->recordTime("writing raster map", rt);

  str->seek(0);

  return;
}





/* laura: this is identical with stream2_FCELL; did not know how to
   template ona type --- is that possible? */
/* ---------------------------------------------------------------------- */
template<class T, class FUN>
void
stream2_CELL(AMI_STREAM<T> *str, dimension_type nrows, dimension_type ncols,
	     FUN fmt, char* cellname) {
  
  
  Rtimer rt;
  AMI_err ae; 
  
  assert(str && cellname);
  /* assert(str->stream_len() == nrows*ncols); */

  rt_start(rt); 
  str->seek(0);
  {
    char * foo;
    str->name(&foo); 
    *stats << "writing stream " << foo << "cellfile  " << cellname << endl;
    fprintf(stderr, "writing stream %s to raster map %s: ", foo, cellname);
  }
  
  /* open output raster map */
  int outfd;
  if ( (outfd = G_open_raster_new (cellname, CELL_TYPE)) < 0) {
    G_fatal_error ("Could not open <%s>", cellname);
  }
  
  /* Allocate output buffer */
  unsigned char *outrast;
  outrast = (unsigned char *)G_allocate_raster_buf(CELL_TYPE);
  assert(outrast);
  
  T* elt;
  ae = str->read_item(&elt);
  assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
  for (int i=0; i< nrows; i++) {
    for (int j=0; j< ncols; j++) {
      
      if(ae == AMI_ERROR_NO_ERROR && elt->i == i && elt->j == j) {
	/* WRITE VALUE */
	if (is_nodata ( fmt(*elt) )) {
	  G_set_c_null_value( &( ((CELL *) outrast)[j]), 1);
	} else { 
	  ((CELL *) outrast)[j] = (CELL)(fmt(*elt));
	}
	ae = str->read_item(&elt);
	assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);

      } else {
	/* WRITE NODATA */
	G_set_c_null_value( &( ((CELL *) outrast)[j]), 1);
      }
      
    } /* for j*/
    if (G_put_raster_row (outfd, outrast, CELL_TYPE) < 0)
      G_fatal_error ("Cannot write to <%s>",cellname);

    G_percent(i, nrows, 2);
  }/* for i */
  G_percent(1, 1, 2); /* finish it */

  G_free(outrast);
  G_close_cell (outfd);

  rt_stop(rt);
  stats->recordTime("writing raster map", rt);

  str->seek(0);
  return;
}



/* ---------------------------------------------------------------------- */
template<class T, class FUN>
void
stream2_FCELL(AMI_STREAM<T> *str, dimension_type nrows, dimension_type ncols,
	     FUN fmt, char* cellname) {
  
  
  Rtimer rt;
  AMI_err ae; 
  
  assert(str && cellname);
  /* assert(str->stream_len() == nrows*ncols); */

  rt_start(rt); 
  str->seek(0);
  {
    char * foo;
    str->name(&foo); 
    *stats << "writing stream " << foo << "cellfile  " << cellname << endl;
    fprintf(stderr, "writing stream %s to raster map %s: ", foo, cellname);
  }
  
  /* open output raster map */
  int outfd;
  if ( (outfd = G_open_raster_new (cellname, FCELL_TYPE)) < 0) {
    G_fatal_error ("Could not open <%s>", cellname);
  }
  
  /* Allocate output buffer */
  unsigned char *outrast;
  outrast = (unsigned char *)G_allocate_raster_buf(FCELL_TYPE);
  assert(outrast);
  
  T* elt;
  ae = str->read_item(&elt);
  assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
  for (int i=0; i< nrows; i++) {
    for (int j=0; j< ncols; j++) {
      
      if(ae == AMI_ERROR_NO_ERROR && elt->i == i && elt->j == j) {
	/* WRITE VALUE */
	if (is_nodata ( fmt(*elt) )) {
	  G_set_f_null_value( &( ((FCELL *) outrast)[j]), 1);
	} else { 
	  ((FCELL *) outrast)[j] = (FCELL)(fmt(*elt));
	}
	ae = str->read_item(&elt);
	assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);

      } else {
	/* WRITE NODATA */
	G_set_f_null_value( &( ((FCELL *) outrast)[j]), 1);
      }
      
    } /* for j*/
    if (G_put_raster_row (outfd, outrast, FCELL_TYPE) < 0)
      G_fatal_error ("Cannot write to <%s>",cellname);

    G_percent(i, nrows, 2);
  }/* for i */
  G_percent(1, 1, 2); /* finish it */

  G_free(outrast);
  G_close_cell (outfd);

  rt_stop(rt);
  stats->recordTime("writing raster map", rt);

  str->seek(0);
  return;
}


/* ---------------------------------------------------------------------- */
/* outstr is sorted by (i,j); 
   
   class sweepOutput {
   public:
   dimension_type i,j; 
   flowaccumulation_type   accu;    
   #ifdef OUTPUT_TCI
   tci_type     tci;      
   #endif
   };
   
create an accu raster map, and a tci raster map if OUTPUT_TCI is defined
*/
template<class T, class FUN1, class FUN2>
void 
stream2_FCELL(AMI_STREAM<T>* str,  dimension_type nrows, dimension_type ncols, 
	      FUN1 fmt1, FUN2 fmt2,
	      char* cellname1, char* cellname2) {
  Rtimer rt;
  AMI_err ae; 

  
  assert(str);
#ifndef   OUTPUT_TCI 
  /* this function should be used only if tci is wanted as output */
  fprintf(stderr, "use this function only if tci is wanted as output\n");
  exit(1);
#else 
  rt_start(rt); 
  
  str->seek(0);
  {
    char * foo;
    str->name(&foo); 
    *stats << "writing stream " << foo << "to raster maps  "
	   << cellname1 << ", " << cellname2 << endl;
    fprintf(stderr, "writing stream %s to raster maps %s, %s: ", 
	    foo, cellname1, cellname2);
  }

  /* open  raster maps */
  int fd1;
  if ( (fd1 = G_open_raster_new (cellname1, FCELL_TYPE)) < 0) {
    G_fatal_error ("Could not open <%s>", cellname1);
  }
  int fd2;
  if ( (fd2 = G_open_raster_new (cellname2, FCELL_TYPE)) < 0) {
    G_fatal_error ("Could not open <%s>", cellname2);
  }
  

  /* Allocate output buffers */
  FCELL *rast1;
  rast1 = (FCELL*)G_allocate_raster_buf(FCELL_TYPE);
  assert(rast1);
  FCELL *rast2;
  rast2 = (FCELL*)G_allocate_raster_buf(FCELL_TYPE);
  assert(rast2);

  T* elt;
  ae = str->read_item(&elt);
  assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
  for (int i=0; i< nrows; i++) {
    for (int j=0; j< ncols; j++) {
      
      if(ae == AMI_ERROR_NO_ERROR && elt->i == i && elt->j == j) {
	/* WRITE VALUE */
	if (is_nodata(fmt1(*elt))) {
	  G_set_f_null_value(&(rast1[j]), 1);
	} else { 
	  rast1[j] = fmt1(*elt);
	};
	if (is_nodata( fmt2(*elt))) {
	  G_set_f_null_value(&(rast2[j]), 1);
	} else { 
	  rast2[j] = fmt2(*elt);
	}
	/* read next value */
	ae = str->read_item(&elt);
	assert(ae == AMI_ERROR_NO_ERROR || ae == AMI_ERROR_END_OF_STREAM);
      

      } else { 
	/* WRITE NODATA */
	G_set_f_null_value(&(rast1[j]), 1);
	G_set_f_null_value(&(rast2[j]), 1);
      }

    } /* for j*/

    if (G_put_raster_row (fd1, rast1, FCELL_TYPE) < 0)
      G_fatal_error ("Cannot write to <%s>", cellname1);
    if (G_put_raster_row (fd2, rast2, FCELL_TYPE) < 0)
      G_fatal_error ("Cannot write to <%s>", cellname2);
    
    G_percent(i, nrows, 2);

  }/* for i */
  G_percent(1, 1, 2); /* finish it */

  G_free(rast1);
  G_close_cell (fd1);
  G_free(rast2);
  G_close_cell (fd2);

  
  rt_stop(rt);
  stats->recordTime("writing stream to raster maps", rt);

  str->seek(0);
  return;
#endif
}


#endif
