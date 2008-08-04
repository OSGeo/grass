
/**
 * \file IPC.c
 *
 * \brief implementation of interprocess communication
 *	primitives between r.li.daemon and r.li.worker
 *
 *
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 *
 * \author Lucio Davide Spano
 * 
 * \version 1.0
 * 
 */


#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "ipc.h"


int send(int pipe, msg * m)
{
    int check;

    /* write on pipe */
    check = write(pipe, m, sizeof(msg));
    if (check > 0)
	return 1;
    else
	return 0;
}

int receive(int pipe, msg * m)
{
    return read(pipe, m, sizeof(msg));
}

void printMsg(msg m)
{

    switch (m.type) {
    case AREA:{
	    G_message(_("				AREA MESSAGE: \n \
				aid = %i \n \
				x = %i \n \
				y = %i \n \
				rl = %i \n \
				cl = %i \n "), m.f.f_a.aid, m.f.f_a.x, m.f.f_a.y, m.f.f_a.rl, m.f.f_a.cl);
	}
	break;
    case MASKEDAREA:{
	    G_message(_(" 				MASKEDAREA MESSAGE: \n \
				aid = %i \n \
				x = %i \n \
				y = %i \n \
				rl = %i \n \
				cl = %i \n \
				mask = %s \n "),
		      m.f.f_ma.aid, m.f.f_ma.x, m.f.f_ma.y, m.f.f_ma.rl, m.f.f_ma.cl, m.f.f_ma.mask);
	}
	break;
    case DONE:{
	    G_message(_(" 				DONE MESSAGE: \n \
				aid = %i \n \
				pid = %i \n \
				result = %f \n "), m.f.f_d.aid, m.f.f_d.pid, m.f.f_d.res);
	}
	break;
    case ERROR:{
	    G_message(_(" 				ERROR MESSAGE: \n \
				aid = %i \n \
				pid = %i \n "), m.f.f_e.aid, m.f.f_e.pid);
	}
	break;
    case TERM:{
	    G_message(_(" 				TERM MESSAGE: \n \
				pid = %i \n "), m.f.f_t.pid);
	}
	break;
    }
}
