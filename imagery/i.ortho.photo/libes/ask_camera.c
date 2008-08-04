
/*************************************************************
* I_ask_camera_old (prompt,camera)
* I_ask_camera_new (prompt,camera)
* I_ask_camera_any (prompt,camera)
*
* prompt the user for an camera reference file name
*************************************************************/
#include <string.h>
#include "orthophoto.h"
#include <grass/ortholib.h>

static int ask_camera(char *, char *);

int I_ask_camera_old(char *prompt, char *camera)
{
    while (1) {
	if (*prompt == 0)
	    prompt = "Select an camera reference file";
	if (!ask_camera(prompt, camera))
	    return 0;
	if (I_find_camera(camera))
	    return 1;
	fprintf(stderr, "\n** %s - not found **\n\n", camera);
    }
}

int I_ask_camera_new(char *prompt, char *camera)
{
    while (1) {
	if (*prompt == 0)
	    prompt = "Enter a new camera reference file name";
	if (!ask_camera(prompt, camera))
	    return 0;
	if (!I_find_camera(camera))
	    return 1;
	fprintf(stderr, "\n** %s - exists, select another name **\n\n",
		camera);
    }
}

int I_ask_camera_any(char *prompt, char *camera)
{
    if (*prompt == 0)
	prompt = "Enter a new or existing camera reference file";
    return ask_camera(prompt, camera);
}

static int ask_camera(char *prompt, char *camera)
{
    char buf[1024];

    while (1) {
	fprintf(stderr, "\n%s\n", prompt);
	fprintf(stderr, "Enter 'list' for a list of existing camera files\n");
	fprintf(stderr, "Enter 'list -f' for a verbose listing\n");
	fprintf(stderr, "Hit RETURN %s\n", G_get_ask_return_msg());
	fprintf(stderr, "> ");
	if (!G_gets(buf))
	    continue;

	G_squeeze(buf);
	fprintf(stderr, "<%s>\n", buf);
	if (*buf == 0)
	    return 0;

	if (strcmp(buf, "list") == 0)
	    I_list_cameras(0);
	else if (strcmp(buf, "list -f") == 0)
	    I_list_cameras(1);
	else if (G_legal_filename(buf) < 0)
	    fprintf(stderr, "\n** <%s> - illegal name **\n\n", buf);
	else
	    break;
    }
    strcpy(camera, buf);
    return 1;
}
