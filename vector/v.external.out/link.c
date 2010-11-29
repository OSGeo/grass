#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

void make_link(const char *dir, const char *ext,
	       const char *format, char **options)
{
    struct Key_Value *key_val = G_create_key_value();
    char *opt_str = NULL;
    FILE *fp;

    if (options && *options) {
	int n_opts = 0, opt_len = 0, i;
	char *p;

	for (i = 0; options[i]; i++) {
	    n_opts++;
	    opt_len += strlen(options[i]) + 1;
	}

	opt_str = G_malloc(opt_len);
	p = opt_str;

	for (i = 0; i < n_opts; i++) {
	    if (i > 0)
		*p++ = ',';
	    strcpy(p, options[i]);
	    p += strlen(options[i]);
	}
	*p++ = '\0';
    }

    if (ext && ext[0] != '.') {
	char *p;

	G_asprintf(&p, ".%s", ext);
	ext = p;
    }

    if (dir)
	G_set_key_value("directory", dir, key_val);
    if (ext)
	G_set_key_value("extension", ext, key_val);
    if (format)
	G_set_key_value("format", format, key_val);
    if (opt_str)
	G_set_key_value("options", opt_str, key_val);

    fp = G_fopen_new("", "OGR");
    if (!fp)
	G_fatal_error(_("Unable to create OGR file"));

    if (G_fwrite_key_value(fp, key_val) < 0)
	G_fatal_error(_("Error writing OGR file"));

    fclose(fp);
}
