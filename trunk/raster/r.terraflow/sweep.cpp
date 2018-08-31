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


#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <grass/iostream/ami.h>

#include "option.h"
#include "stats.h"
#include "sweep.h"
#include "common.h"
#include "weightWindow.h"
#include "nodata.h"
#include "sortutils.h"


/* frequency; used to print progress dots */
static const int DOT_CYCLE = 50;
static const int PQSIZE_CYCLE = 100;


/* globals in common.H

extern statsRecorder *stats;       stats file 
extern userOptions *opt;           command-line options 
extern struct  Cell_head *region;  header of the region 
extern dimension_type nrows, ncols;
*/



/* SELECT FLOW DATA STRUCTURE */
#ifdef IM_PQUEUE
typedef pqheap_t1<flowStructure> FLOW_DATASTR;
#endif
#ifdef EM_PQUEUE
typedef em_pqueue<flowStructure, flowPriority> FLOW_DATASTR;
#endif
#ifdef EMPQ_ADAPTIVE
typedef EMPQueueAdaptive<flowStructure, flowPriority> FLOW_DATASTR;
#endif


/* defined in this module */
void pushFlow(const sweepItem& swit, const flowValue &flow, 
	      FLOW_DATASTR* flowpq, const weightWindow &weight);




/* ------------------------------------------------------------*/
sweepOutput::sweepOutput() {
  i = (dimension_type) nodataType::ELEVATION_NODATA;
  j = (dimension_type) nodataType::ELEVATION_NODATA;
  accu = (flowaccumulation_type) nodataType::ELEVATION_NODATA;  
#ifdef OUTPUT_TCI
  tci = (tci_type) nodataType::ELEVATION_NODATA;
#endif
};


/* ------------------------------------------------------------ */
/* computes output parameters of cell (i,j) given the flow value, the
   elevation of that cell and the weights of that cell; */
void
sweepOutput::compute(elevation_type elev, 
		    dimension_type i_crt, dimension_type j_crt, 
		    const flowValue &flow, 
		    const weightWindow &weight, 
		    const  elevation_type nodata) {
  
  float correct_tci; /* this the correct value of tci; we're going to
  truncate this on current precision tci_type set by user in types.H*/

  assert(elev != nodata);
  assert(flow.get() >= 0);
  assert(weight.sumweight >= 0 && weight.sumcontour >= 0);

  i = i_crt;
  j = j_crt;
  
  if (weight.sumweight == 0 || weight.sumcontour == 0) {
    accu = (flowaccumulation_type)nodata;
#ifdef OUTPUT_TCI
    tci = (tci_type)nodata;
#endif
    
  } else {
    accu = flow.get();
#ifdef OUTPUT_TCI
    correct_tci = log(flow.get()*weight.dx()*weight.dy()/weight.totalContour());
    /* assert(correct_tci > 0); //is this true? */
    /* there is no need for this warning. tci can be negative if the flow is small. */
    /* if (correct_tci < 0) {
       G_warning(tci negative, [flow=%f,dx=%f,dy=%f,cont=%f]\n",
       flow.get(), weight.dx(), weight.dy(), weight.totalContour());
       }
    */
    tci = (tci_type)correct_tci;
#endif
  }
  
  return;
}




FLOW_DATASTR* 
initializePQ() {
   
  if (stats)
    stats->comment("sweep:initialize flow data structure", opt->verbose);
  
  FLOW_DATASTR *flowpq;
#ifdef IM_PQUEUE
  if (stats)
    stats->comment("FLOW_DATASTRUCTURE: in-memory pqueue");
  flowpq = new FLOW_DATASTR(PQ_SIZE);
  char buf[1024]; 
  sprintf(buf, "initialized to %.2fMB\n", (float)PQ_SIZE / (1<<20));
  if (stats)
    *stats << buf; 

#endif
#ifdef EM_PQUEUE
  if (stats)
    stats->comment("FLOW_DATASTRUCTURE: ext-memory pqueue");
  flowpq = new FLOW_DATASTR(nrows * ncols);  
#endif
#ifdef EMPQ_ADAPTIVE
  if (opt->verbose && stats) stats->comment("FLOW_DATASTRUCTURE: adaptive pqueue");
  flowpq = new FLOW_DATASTR(); 
#endif
  return flowpq;
}
  
/***************************************************************/
/* Read the points in order from the sweep stream and process them.
   If trustdir = 1 then trust and use the directions contained in the
   sweep stream. Otherwise push flow to all downslope neighbors and
   use the direction only for points without downslope neighbors. */
/***************************************************************/

AMI_STREAM<sweepOutput>* 
sweep(AMI_STREAM<sweepItem> *sweepstr, const flowaccumulation_type D8CUT,
      const int trustdir) {
  flowPriority prio;
  flowValue flow;
  sweepItem* crtpoint;
  AMI_err ae;
  flowStructure x;	
  long nitems;
  Rtimer rt;
  AMI_STREAM<sweepOutput>* outstr;

  rt_start(rt);

  assert(sweepstr);

  if (stats)
    *stats << "sweeping\n";
  G_debug(1, "sweeping: ");
  /* create and initialize flow data structure */
  FLOW_DATASTR *flowpq;
  flowpq = initializePQ();
  
  /* create output stream */
  outstr = new AMI_STREAM<sweepOutput>();
  
  /* initialize weights and output */
  weightWindow weight(region->ew_res, region->ns_res);
  sweepOutput output;
  nitems = sweepstr->stream_len();

#ifndef NDEBUG
  flowPriority prevprio = flowPriority(SHRT_MAX);	/* XXX      */
#endif
  /* scan the sweep stream  */
  ae = sweepstr->seek(0);
  assert(ae == AMI_ERROR_NO_ERROR);
  G_important_message(_("Sweeping..."));
  for (long k = 0; k < nitems; k++) {
    
    /* cout << k << endl; cout.flush(); */
    /* read next sweepItem = (prio, elevwin, topoRankwin, dir) */
    ae = sweepstr->read_item(&crtpoint);
    if (ae != AMI_ERROR_NO_ERROR) {
      fprintf(stderr, "sweep: k=%ld: cannot read next item..\n", k);
      exit(1);
    }
    /* cout << "k=" << k << " prio =" << crtpoint->getPriority() << "\n"; */
    /* nodata points should not be in sweep stream */
    assert(!is_nodata(crtpoint->getElev()));
#ifndef NDEBUG    
    assert(crtpoint->getPriority() > prevprio); /* XXX */
    prevprio = crtpoint->getPriority(); /* XXX */
#endif
    
    
    /* compute flow accumulation of current point; initial flow value
       is 1 */
    flowValue flowini((double)1);
    /* add flow which was pushed into current point by upslope
       neighbours */
    assert(flowpq->is_empty() || 
		   (flowpq->min(x), x.getPriority() >= crtpoint->getPriority())); /* XXX */
    assert(flowpq->is_empty() != flowpq->min(x));	/* XXX */
    if (flowpq->min(x) && ((prio=x.getPriority()) == crtpoint->getPriority())) {
      flowpq->extract_all_min(x);
      /* cout << "EXTRACT: " << x << endl; */
      flow = x.getValue();
      flow = flow + flowini;
    } else {
      flow = flowini;
    }
    assert(flowpq->is_empty() || 
		   (flowpq->min(x), x.getPriority() > crtpoint->getPriority())); /* XXX */
	


    /* compute weights of current point given its direction */
    if (flow > D8CUT) {
      /* consider just the dominant direction */
      weight.makeD8(crtpoint->getI(), crtpoint->getJ(), 
		    crtpoint->getElevWindow(), crtpoint->getDir(), trustdir);
    } else {
      /* consider multiple flow directions */
      weight.compute(crtpoint->getI(), crtpoint->getJ(), 
		     crtpoint->getElevWindow(), crtpoint->getDir(), trustdir);
    }    
    
    
    /* distribute the flow to its downslope neighbours  */
    pushFlow(*crtpoint, flow, flowpq, weight);

    
    /* compute parameters  */
    output.compute(crtpoint->getElev(), crtpoint->getI(), crtpoint->getJ(),
		   flow, weight, nodataType::ELEVATION_NODATA);
#ifdef CHECKPARAM   
    printf("%7ld: (%5d, %5d, %5d) flow: %7.3f, weights:[",
	   k, crtpoint->getElev(), crtpoint->getI(),crtpoint->getJ(), 
	   flow.get());
    for (int l=0;l<9;l++) printf("%2.1f ",weight.get(l));
    cout <<"] ";
    cout << output << "\n";
#endif        

    /* write output to sweep output stream */
    ae = outstr->write_item(output);
    assert(ae == AMI_ERROR_NO_ERROR);
    
    G_percent(k, nitems, 2);
  } /* for k  */
  
  G_percent(1, 1, 1); /* finish it */

  if (stats)
    *stats << "sweeping done\n";
  char buf[1024];
  sprintf(buf, "pqsize = %ld \n", (long)flowpq->size());
  if (stats)
    *stats << buf;
  
  assert(outstr->stream_len() == nitems);
  delete flowpq; 

  rt_stop(rt);
  if (stats) {
      stats->recordTime("sweeping", rt);
      stats->recordLength("sweep output stream", outstr);
  }

  return outstr;
}







/***************************************************************/
/* push flow to neighbors as indicated by flow direction and reflected
   by the weights of the neighbors; flow is the accumulated flow value
   of current point; The neighbours which receive flow from current
   point are inserted in the FLOW_DATASTR */
/***************************************************************/
void 
pushFlow(const sweepItem& swit, const flowValue &flow, 
	 FLOW_DATASTR *flowpq, 
	 const weightWindow &weight) {
  
  dimension_type  i_crt, j_crt, i_neighb, j_neighb;
  short di, dj;
  elevation_type elev_crt, elev_neighb;
  toporank_type toporank_crt;
 
  assert(flow >= 0);
  /* get current coordinates, elevation, topological rank */
  i_crt = swit.getI(); 
  j_crt = swit.getJ();
  elev_crt = swit.getElev();
  toporank_crt = swit.getTopoRank();
  assert(!is_nodata(elev_crt)); 

  for (di = -1; di <= 1; di++) {
    for (dj = -1; dj <= 1; dj++) {
      if (weight.get(di,dj) > 0) {

		/* push flow to this neighbor  */
	i_neighb = i_crt + di;  
	j_neighb = j_crt + dj;
	elev_neighb = swit.getElev(di,dj);
	
	/*assert(IS_BOUNDARY(i_crt,j_crt,hdr) || elev_neighb !=hdr.get_nodata());*/
	/* it's not simple to check what nodata is on boundary, so we'll
	just assume directions are correct even if they point to nodata
	elevation values. */

	if (!is_nodata(elev_neighb)) {
	  flowPriority prio(elev_neighb, swit.getTopoRank(di,dj), 
			    i_neighb, j_neighb);
	  flowPriority prio_crt(elev_crt,toporank_crt, i_crt, j_crt);
	  /* assert(prio >= prio_crt); */
#if (defined WARNING_FLAG)
	  if (prio < prio_crt) {
	    printf("\n(row=%d,col=%d,ele=%d): ",
		   i_crt, j_crt, elev_crt);
	    cout << "attempt to push flow uphill\n";
	  }
#endif
	  flowValue elt(weight.get(di,dj)*flow.get());
	  flowStructure x(prio, elt);
	  assert(x.getPriority() > swit.getPriority());
	  flowpq->insert(x);
	  /* cout << "INSERT: " << x << endl;  */
	} /* if (!is_nodata(elev_neighb)) */
      }
    } /* for dj */
  } /* for di */
  return;
}






