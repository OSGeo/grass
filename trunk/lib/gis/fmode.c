#include <stdlib.h>
#include <fcntl.h>
#undef _fmode
int _fmode = _O_BINARY;
