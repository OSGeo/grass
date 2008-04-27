#include <string.h>
#include <stdlib.h>
#include <grass/dbmi.h>

#include <errno.h>

static int  err_flag = 0;
static int  err_code = DB_OK;
static char *err_msg = 0;
static int  auto_print_errors = 1;
static int auto_print_protocol_errors = 1;
static void (*user_print_function)();

static char *who = NULL;

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_on_error (void (*f)())

{
    user_print_function = f;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_set_error_who (char *me)

{
    if (who) free(who);
    who = db_store(me);
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
char *
db_get_error_who()
{
    return who?who:"";
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_error (char *s)

{
    if (s == NULL)
	s = "<NULL error message>";
    if(err_msg)
	free(err_msg);
    err_msg =  db_store(s);
    err_flag = 1;
    if (auto_print_errors)
	db_print_error();
    err_code = DB_FAILED;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_protocol_error()
{
    int flag;

    flag = auto_print_errors;
    auto_print_errors = auto_print_protocol_errors;
    db_error ("dbmi: Protocol error");
    auto_print_errors = flag;
    err_code = DB_PROTOCOL_ERR;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_syserror (char *s)

{
    char lead[1024];
    char msg[1024];


    err_flag = 0;
    if (errno <= 0) return;

    *lead = 0;
    if (who)
	sprintf (lead, "%s: ", who);

    if (errno > 0)
	sprintf (msg, "%s%s: %s", lead, strerror(errno), s);

    db_error (msg);
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
int
db_get_error_code()
{
    return err_flag ? err_code : DB_OK;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_memory_error()
{
    db_error ("dbmi: Out of Memory");
    err_code = DB_MEMORY_ERR;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_procedure_not_implemented (char *name)

{
    char msg[128];

    sprintf (msg, "dbmi: %s() not implemented", name);
    db_error (msg);
    err_code = DB_NOPROC;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_noproc_error(procnum)
{
    char msg[128];

    sprintf (msg, "dbmi: Invalid procedure %d", procnum);
    db_error (msg);
    err_code = DB_NOPROC;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_clear_error()
{
    err_flag = 0;
    err_code = DB_OK;
    errno    = 0;	/* clearn system errno as well */
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_print_error()
{
    char lead[1024];

    if(!err_flag) return;

    *lead = 0;
    if (who)
	sprintf (lead, "%s: ", who);

    if (user_print_function) {
        char buf[1024];
        sprintf(buf, "%s%s\n", lead, err_msg);
        user_print_function(buf);
    }
    else
        fprintf (stderr, "%s%s\n", lead, err_msg);
}


static int debug_on = 0;

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_debug_on()
{
    debug_on = 1;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_debug_off()
{
    debug_on = 0;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_debug (char *s)

{
    if (debug_on)
	fprintf (stderr, "debug(%s): %s\n", who?who:"", s?s:"<NULL>");
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
char *
db_get_error_msg()
{
    return err_flag ? err_msg : (char *)NULL;
}

/*!
 \fn void db_auto_print_errors (flag)
 \brief toggles printing of DBMI error messages
 \return void
 \param flag
*/
void
db_auto_print_errors (flag)
{
    auto_print_errors = flag;
    auto_print_protocol_errors = flag;
}

/*!
 \fn 
 \brief 
 \return 
 \param 
*/
void
db_auto_print_protocol_errors (flag)
{
    auto_print_protocol_errors = flag;
}
