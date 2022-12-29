#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "proto.h"

#include <grass/spawn.h>
#include <grass/glocale.h>

int reinvoke_script(const struct context *ctx, const char *filename)
{
    struct Option *option;
    struct Flag *flag;

    /* Because shell from MINGW and CygWin converts all variables
     * to uppercase it was necessary to use uppercase variables.
     * Set both until all scripts are updated */
    for (flag = ctx->first_flag; flag; flag = flag->next_flag) {
	char buff[16];

	sprintf(buff, "GIS_FLAG_%c=%d", flag->key, flag->answer ? 1 : 0);
	putenv(G_store(buff));

	sprintf(buff, "GIS_FLAG_%c=%d", toupper(flag->key),
		flag->answer ? 1 : 0);

	G_debug(2, "set %s", buff);
	putenv(G_store(buff));
    }

    for (option = ctx->first_option; option; option = option->next_opt) {
	char upper[4096];
	char *str;

	G_asprintf(&str, "GIS_OPT_%s=%s", option->key,
		   option->answer ? option->answer : "");
	putenv(str);

	strcpy(upper, option->key);
	G_str_to_upper(upper);
	G_asprintf(&str, "GIS_OPT_%s=%s", upper,
		   option->answer ? option->answer : "");

	G_debug(2, "set %s", str);
	putenv(str);
    }

#ifdef __MINGW32__
    {
	/* execlp() and _spawnlp ( _P_OVERLAY,..) do not work, they return 
	 * immediately and that breaks scripts running GRASS scripts
	 * because they dont wait until GRASS script finished */
	/* execlp( "sh", "sh", filename, "@ARGS_PARSED@", NULL); */
	/* _spawnlp ( _P_OVERLAY, filename, filename, "@ARGS_PARSED@", NULL ); */
	int ret;
	char *shell = getenv("GRASS_SH");

	if (shell == NULL)
	    shell = "sh";
	ret = G_spawn(shell, shell, filename, "@ARGS_PARSED@", NULL);
	G_debug(1, "ret = %d", ret);
	if (ret == -1) {
	    perror(_("G_spawn() failed"));
	    return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
    }
#else
    execl(filename, filename, "@ARGS_PARSED@", NULL);

    perror(_("execl() failed"));
    return EXIT_FAILURE;
#endif
}
