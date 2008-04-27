#include <stdio.h>
#include "local_proto.h"

int graphics (void)
{
	char buf[128] ;
	int more;

	more = read_line(buf, sizeof(buf));

	while(more)
	{
		switch (*buf & 0177)
		{
		case 't':
			do_text(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 's':
			do_size(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 'p':
			more = do_poly(buf, sizeof(buf)) ;
			break ;
		case 'c':
			do_color(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 'm':
			do_move(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 'd':
			do_draw(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 'i':
			do_icon(buf) ;
			more = read_line(buf, sizeof(buf));
			break ;
		case 0:
		case '#':
			more = read_line(buf, sizeof(buf));
			break ;
		default:
			bad_line (buf);
			more = read_line(buf, sizeof(buf));
			break ;
		}
	}

	return 0;
}
