#include "builtin.h"
#include "runcmd.h"
#include "history_file.h"

void exec_history(char *argv[]);

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	int boolean = 0;

	if (strcmp(cmd, EXIT_CMD_STR) == 0) {
		boolean = 1;
	}

	return boolean;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, CD_CMD_STR, strlen(CD_CMD_STR)) != 0) {
		return 0;
	}

	int chdir_result = -1;
	int boolean = 0;

	// If the input is only ['c', 'd', '\0'] redirect to $HOME
	if (strlen(cmd) == 2 && strcmp(cmd, CD_CMD_STR) == 0) {
		chdir_result = chdir(getenv(HOME_ENV_NAME));
	} else {
		// Get all values after 'c','d',' '
		char *pointer = cmd + 3;
		chdir_result = chdir(pointer);
	}

	// If there is no error, update prompt
	if (chdir_result == 0) {
		boolean = 1;
		char *update_result = getcwd(prompt, sizeof(prompt));
		char *temp_prompt = strdup(prompt);
		strcpy(prompt, "(");
		strcat(prompt, temp_prompt);
		strcat(prompt, ")");
		free(temp_prompt);

		// Id there is an error updating prompt
		if (update_result == NULL) {
			fprintf(stderr,
			        "Error updating prompt value after command %s",
			        cmd);
			exit(-1);
		}
	}

	return boolean;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, PWD_CMD_STR) != 0) {
		return 0;
	}

	int boolean = 0;
	char working_dir[PRMTLEN];

	if (getcwd(working_dir, sizeof(working_dir)) != NULL) {
		boolean = 1;
		printf("%s\n", working_dir);
	}

	return boolean;
}

int
event_designator_prev(char *cmd)
{
	if (strcmp(cmd, EVENT_DESIGNATOR_PREV) != 0) {
		return 0;
	}

	char path_history[BUFLEN];
	get_history_file_path(path_history);

	// No hay historia
	FILE *history = fopen(path_history, "r");
	if (!history) {
		return 1;
	}

	HistoryResult *history_result = read_from_history(history, 0);
	printf("%s\n", history_result->lines[history_result->size - 1]);
	run_cmd(history_result->lines[history_result->size - 1]);
	return 1;
}

int
event_designator_n(char *cmd)
{
	if (cmd[0] != EVENT_DESIGNATOR) {
		return 0;
	}

	int n = atoi(cmd + 2);
	if (n == 0) {
		return 0;
	}

	char path_history[BUFLEN];
	get_history_file_path(path_history);

	// No hay historia
	FILE *history = fopen(path_history, "r");
	if (!history) {
		return 1;
	}

	HistoryResult *history_result = read_from_history(history, 0);
	if (n >= (int) history_result->size) {
		return 1;
	}

	printf("%s\n", history_result->lines[n - 1]);
	run_cmd(history_result->lines[n - 1]);
	return 1;
}

// returns true if 'history' was invoked
// in the command line
//
// (It has to be executed here and then
//		return true)
int
history(char *cmd)
{
	if (strncmp(cmd, HISTORY_CMD_STR, strlen(HISTORY_CMD_STR)) != 0) {
		return 0;
	}

	char path_history[BUFLEN];
	get_history_file_path(path_history);

	FILE *history = fopen(path_history, "r");
	if (!history) {
		return -1;
	}

	HistoryResult *history_result = NULL;
	if (strcmp(cmd, HISTORY_CMD_STR) == 0) {
		history_result = read_from_history(history, 0);
	} else {
		int n = atoi(cmd + strlen(HISTORY_CMD_STR));
		if (n < 0) {
			fprintf(stdout, "history: %d: invalid option\n", n);
			fclose(history);
			return -1;
		} else if (n > 0) {
			history_result = read_from_history(history, n);
		}
	}

	// If file is empty
	if (history_result == NULL) {
		return 1;
	}

	// Print all commands in history
	for (size_t i = 0; i < history_result->size; i++) {
		printf("%s\n", history_result->lines[i]);
	}

	HistoryResult_destroy(history_result);
	fclose(history);
	return 1;
}