#include "globals.h"
#include "local_proto.h"

#define INP_STD 1

float Nstd;
static int use = 1;

int input_std(void)
{
    static Objects objects[] = {
	INFO("Number of Std Deviations: ", &use),
	MENU(" 0.5 ", nstd050, &use),
	MENU(" 0.75 ", nstd075, &use),
	MENU(" 1.0 ", nstd100, &use),
	MENU(" 1.25 ", nstd125, &use),
	MENU(" 1.5 ", nstd150, &use),
	MENU(" 1.75 ", nstd175, &use),
	MENU(" 2.0 ", nstd200, &use),
	MENU(" 2.25 ", nstd225, &use),
	MENU(" 2.5 ", nstd250, &use),
	MENU(" Other ", other, &use),
	{0}
    };

    Input_pointer(objects);
    Menu_msg("");
    return (INP_STD);
}



int other(void)
{
    char tmpstr[50];
    float tempflt;
    int good;

    Menu_msg("Use Keyboard on Text Terminal...");

    do {
	Curses_prompt_gets("Enter Number of Standard Deviations: ", tmpstr);
	good = sscanf(tmpstr, " %f ", &tempflt);
	if (tempflt <= 0.0)
	    good = 0;
    } while (good != 1);

    Nstd = tempflt;
    use_mouse_msg();
    return (1);
}

int nstd050(void)
{
    Nstd = 0.50;
    return (1);
}

int nstd075(void)
{
    Nstd = 0.75;
    return (1);
}

int nstd100(void)
{
    Nstd = 1.0;
    return (1);
}

int nstd125(void)
{
    Nstd = 1.25;
    return (1);
}

int nstd150(void)
{
    Nstd = 1.50;
    return (1);
}

int nstd175(void)
{
    Nstd = 1.75;
    return (1);
}

int nstd200(void)
{
    Nstd = 2.00;
    return (1);
}

int nstd225(void)
{
    Nstd = 2.25;
    return (1);
}

int nstd250(void)
{
    Nstd = 2.50;
    return (1);
}
