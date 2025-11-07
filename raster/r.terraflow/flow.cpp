/****************************************************************************
 *
 *  MODULE:        r.terraflow
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

#include <time.h>
#include <ctype.h>

#include "flow.h"
#include "sweep.h"
#include "option.h"
#include "common.h"
#include "sortutils.h"
#include "streamutils.h"
#include "water.h"
#include "3scan.h"

/* globals in common.H

extern statsRecorder *stats;       stats file
extern userOptions *opt;           command-line options
extern struct  Cell_head *region;  header of the region
extern dimension_type nrows, ncols;
*/

/* defined in this module */
AMI_STREAM<sweepItem> *
fillstr2sweepstr(AMI_STREAM<waterWindowBaseType> *flowStream);

/* ********************************************************************** */
/* deletes fillStream */
void computeFlowAccumulation(AMI_STREAM<waterWindowBaseType> *fillStream,
                             AMI_STREAM<sweepOutput> *&outstr)
{
    Rtimer rt, rtTotal;
    AMI_STREAM<sweepItem> *sweepstr;

    rt_start(rtTotal);
    assert(fillStream && outstr == NULL);
    if (stats) {
        stats->comment("------------------------------");
        stats->comment("COMPUTING FLOW ACCUMULATION");
    }

    { /* timestamp stats file and print memory */
        time_t t = time(NULL);
        char buf[BUFSIZ];
        if (t == (time_t)-1) {
            perror("time");
            exit(1);
        }
#ifdef _WIN32
        strcpy(buf, ctime(&t));
#else
        ctime_r(&t, buf);
        buf[24] = '\0';
#endif
        if (stats) {
            stats->timestamp(buf);
            *stats << endl;
        }

        size_t mm_size = (opt->mem << 20); /* (in bytes) */
        formatNumber(buf, mm_size);
        if (stats)
            *stats << "memory size: " << buf << " bytes\n";
    }

    /* create sweepstream using info from  fillStream  */
    sweepstr = fillstr2sweepstr(fillStream);
    /* fillStream is deleted inside fillstr2sweepstr */

    /* sweep and dump outputs into outStream; trustdir=1 */
    outstr = sweep(sweepstr, opt->d8cut, 1);
    assert(outstr->stream_len() == sweepstr->stream_len());
    delete sweepstr;

    /* sort output stream into a grid */
    rt_start(rt);
    if (stats) {
        stats->comment("sorting sweep output stream");
        stats->recordLength("output stream", outstr);
    }
    sort(&outstr, ijCmpSweepOutput());
    rt_stop(rt);
    if (stats) {
        stats->recordLength("output stream", outstr);
        stats->recordTime("sorting output stream", rt);
    }

    rt_stop(rtTotal);
    if (stats)
        stats->recordTime("compute flow accumulation", rtTotal);

#ifdef SAVE_ASCII
    printStream2Grid(outstr, nrows, ncols, "flowaccumulation.asc",
                     printAccumulationAscii());
    printStream2Grid(outstr, nrows, ncols, "tci.asc", printTciAscii());
#endif
    return;
}

/****************************************************************/
class flow_waterWindower {
private:
    AMI_STREAM<sweepItem> *sweep_str;

public:
    flow_waterWindower(AMI_STREAM<sweepItem> *str) : sweep_str(str) {};
    void processWindow(dimension_type i, dimension_type j,
                       waterWindowBaseType *a, waterWindowBaseType *b,
                       waterWindowBaseType *c);
};

/****************************************************************/
void flow_waterWindower::processWindow(dimension_type i, dimension_type j,
                                       waterWindowBaseType *a,
                                       waterWindowBaseType *b,
                                       waterWindowBaseType *c)
{

    elevation_type el1[3], el2[3], el3[3];
    toporank_type ac1[3], ac2[3], ac3[3];

    if (is_nodata(b[1].el)) {
        /*sweep_str does not include nodata */
        return;
    }
    /*#ifdef  COMPRESSED_WINDOWS
          sweepItem win = sweepItem(i, j, a, b, c);
          #else
    */
    for (int k = 0; k < 3; k++) {
        el1[k] = a[k].el;
        ac1[k] = -a[k].depth; /*WEIRD */
        el2[k] = b[k].el;
        ac2[k] = -b[k].depth; /*WEIRD*/
        el3[k] = c[k].el;
        ac3[k] = -c[k].depth; /*WEIRD*/
    }
    /*
          genericWindow<elevation_type> e_win(el);
          genericWindow<toporank_type> a_win(ac);
          sweepItem win = sweepItem(i, j, b[1].dir, e_win, a_win);
    */
    sweepItem win = sweepItem(i, j, b[1].dir, el1, el2, el3, ac1, ac2, ac3);
    /* #endif */

    AMI_err ae = sweep_str->write_item(win);
    assert(ae == AMI_ERROR_NO_ERROR);
}

/****************************************************************/
void waterWindowBaseType2sweepItem(AMI_STREAM<waterWindowBaseType> *baseStr,
                                   const dimension_type nrows,
                                   const dimension_type ncols,
                                   const elevation_type nodata_value,
                                   AMI_STREAM<sweepItem> *sweep_str)
{
    flow_waterWindower winfo(sweep_str);
    waterWindowBaseType nodata((elevation_type)nodata_value,
                               (direction_type)nodata_value, DEPTH_INITIAL);
    /*
           assert(baseStr->stream_len() > 0);
           XXX - should check if it fits in memory technically don't need to
           give the template args, but seems to help the compiler
           memoryScan(*baseStr, hdr, nodata,  winfo);
    */
    memoryScan<waterWindowBaseType, flow_waterWindower>(*baseStr, nrows, ncols,
                                                        nodata, winfo);
}

/****************************************************************/
/* open fill's output stream and get all info from there; delete
   fillStream */
AMI_STREAM<sweepItem> *
fillstr2sweepstr(AMI_STREAM<waterWindowBaseType> *fillStream)
{

    Rtimer rt;
    AMI_STREAM<sweepItem> *sweepstr;

    rt_start(rt);

    if (stats)
        stats->comment("creating sweep stream from fill output stream");

    assert(fillStream->stream_len() == (off_t)nrows * ncols);

    /* create the sweep stream */
    sweepstr = new AMI_STREAM<sweepItem>();
    waterWindowBaseType2sweepItem(fillStream, nrows, ncols,
                                  nodataType::ELEVATION_NODATA, sweepstr);
    delete fillStream;

    G_debug(1, "sweep stream size: %.2fMB",
            (double)sweepstr->stream_len() * sizeof(sweepItem) / (1 << 20));
    G_debug(1, " (%d items, item size=%ld B\n ", (int)sweepstr->stream_len(),
            sizeof(sweepItem));
    ;

    if (stats)
        stats->recordLength("sweep stream", sweepstr);

    /* sort sweep stream by (increasing) priority */
    G_debug(1, "Sorting sweep stream (%.2fMB) in priority order",
            (double)sweepstr->stream_len() * sizeof(sweepItem) / (1 << 20));
    if (stats)
        stats->comment("sorting sweep stream");
    sort(&sweepstr, PrioCmpSweepItem());

    rt_stop(rt);

    if (stats) {
        stats->recordTime("create sweep stream", rt);
        stats->recordLength("(sorted) sweep stream", sweepstr);
    }

    return sweepstr;
}
