#include "xdr.h"
#include "macros.h"


/******** client only ***************/
int db__start_procedure_call(int procnum)
{
    int reply;

    DB_SEND_INT(procnum);
    DB_RECV_INT(&reply);
    if (reply != procnum) {
	if (reply == 0) {
	    db_noproc_error(procnum);
	}
	else {
	    db_protocol_error();
	}
	return DB_PROTOCOL_ERR;
    }

    return DB_OK;
}

/***** driver only *******************/

/* return codes:
 * DB_OK  ok
 * DB_EOF eof from client
 */
int db__recv_procnum(int *n)
{
    int stat = DB_OK;

    if (!db__recv(n, sizeof(*n)))
	stat = DB_EOF;

    return stat;
}

int db__send_procedure_ok(int n)
{
    return db__send_int(n);
}

int db__send_procedure_not_implemented(int n)
{
    return db__send_int(n ? 0 : -1);
}
