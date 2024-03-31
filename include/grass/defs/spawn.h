#ifndef GRASS_SPAWNDEFS_H
#define GRASS_SPAWNDEFS_H

extern int G_spawn(const char *command, ...);
extern int G_vspawn_ex(const char *command, const char **args);
extern int G_spawn_ex(const char *command, ...);
extern int G_wait(int i_pid);

#endif
