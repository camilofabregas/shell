#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"

#define CD_CMD_STR "cd"
#define PWD_CMD_STR "pwd"
#define EXIT_CMD_STR "exit"
#define HOME_ENV_NAME "HOME"
#define HISTORY_CMD_STR "history"
#define EVENT_DESIGNATOR '!'
#define EVENT_DESIGNATOR_PREV "!!"

extern char prompt[PRMTLEN];

int cd(char *cmd);

int exit_shell(char *cmd);

int pwd(char *cmd);

int history(char *cmd);

int event_designator_prev(char *cmd);

int event_designator_n(char *cmd);

#endif  // BUILTIN_H
