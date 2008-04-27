/* $Id$ */
#ifndef SHHOPT_H
#define SHHOPT_H

#ifdef __cplusplus
extern "C" {
#endif

/* constants for recognized option types. */
typedef enum {
    OPT_END,               /* nothing. used as ending element. */
    OPT_FLAG,              /* no argument following. sets variable to 1. */
    OPT_STRING,            /* string argument. */
    OPT_INT,               /* signed integer argument. */
    OPT_UINT,              /* unsigned integer argument. */
    OPT_LONG,              /* signed long integer argument. */
    OPT_ULONG,             /* unsigned long integer argument. */
    OPT_FLOAT              /* floating point argument. */
} optArgType;

/* flags modifying the default way options are handeled. */
#define OPT_CALLFUNC  1    /* pass argument to a function. */

    typedef struct {
    char       shortName;  /* short option name. */
    const char *longName;  /* long option name, not including '--'. */
    optArgType type;       /* option type. */
    void       *arg;       /* pointer to variable to fill with argument,
                            * or pointer to function if type == OPT_FUNC. */
    int        flags;      /* modifier flags. */
} optStruct;
    

typedef struct {
    unsigned char short_allowed;  /* boolean */
        /* The syntax may include short (i.e. one-character) options.
           These options may be stacked within a single token (e.g.
           -abc = -a -b -c).  If this value is not true, the short option
           member of the option table entry is meaningless and long 
           options may have either one or two dashes.
           */
    unsigned char allowNegNum;  /* boolean */
        /* Anything that starts with - and then a digit is a numeric
           parameter, not an option 
           */
    optStruct *opt_table;
} optStruct2;
        
void optSetFatalFunc(void (*f)(const char *, ...));
void optParseOptions(int *argc, char *argv[],
		     optStruct opt[], int allowNegNum);
void
optParseOptions2(int * const argc_p, char *argv[], const optStruct2 opt, 
                 const unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif
