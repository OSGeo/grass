#include <grass/gis.h>

#include <grass/config.h>
#ifndef __MINGW32__
#if defined(HAVE_TERMIOS_H)
# include <termios.h>
# define TYPE termios
# define C c_cc[VINTR]
#elif defined(HAVE_TERMIO_H)
# include <termio.h>
# define TYPE termio
# define C c_cc[VINTR]
# define GET TCGETA
#else
# include <sgtty.h>
# define TYPE tchars
# define C t_intrc
# define GET TIOCGETC
#endif
#endif

/*!
 * \brief return interrupt char
 *
 * This routine returns the
 * user's keyboard interrupt character. This is the character that generates the
 * SIGINT signal from the keyboard.
 *
 *  \param ~
 *  \return char 
 */

char G_intr_char(void)
{
    char c = 0;

#ifndef __MINGW32__
    struct TYPE buf;

#ifdef HAVE_TERMIOS_H
    tcgetattr(2, &buf);
#else
    ioctl(2, GET, &buf);
#endif
    c = buf.C;
#endif
    return c;
}
