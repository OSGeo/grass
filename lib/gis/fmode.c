// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdlib.h>
#include <fcntl.h>
#undef _fmode
int _fmode = _O_BINARY;
