#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2)
	return 1;
    fputs(argv[1], stdout);
    return 0;
}
