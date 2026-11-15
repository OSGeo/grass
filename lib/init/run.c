/****************************************************************
this program runs its arguments as  a  command.  it  essentially
does what the sh would do to look for the command. if / occurs in
the command it runs it  as  is,  otherwise  it  search  the  PATH
variable.  care  is  taken  to preserve the PATH variable that is
passed (as part of the environment) to the command being invoked.

the signals SIGINT and SIGQUIT are  set  to  the  default  action
before running the command.

This  program  is  needed  because  the  GIS  shell  must  ignore
interrupts when it runs the user's shell. There is no way to tell
the user's shell to re-activate interrupts in shell-ese.
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "local_proto.h"

int main(int argc, char *argv[])
{
    signal(SIGINT, SIG_DFL);
#ifndef _WIN32
    signal(SIGQUIT, SIG_DFL);
#endif

    argc--;
    argv++;
    if (argc <= 0)
        exit(1);

    execvp(argv[0], argv);
    fprintf(stderr, "%s: Command not found\n", argv[0]);
    exit(127);

    exit(0);
}
