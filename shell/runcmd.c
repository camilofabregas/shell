#include "runcmd.h"
#include "history_file.h"
#include "noncanonical.h"

int status = 0;
struct cmd *parsed_pipe;

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

		// "history" built-in call
#ifndef SHELL_NO_INTERACTIVE
	if (event_designator_prev(cmd))
		return 0;

	if (event_designator_n(cmd))
		return 0;

	if (history(cmd))
		return 0;
	write_to_history(cmd);
#endif

	// "cd" built-in call
	if (cd(cmd))
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	if (parsed->type == BACK) {
		print_back_info(parsed);
	} else {  // waits for the process to finish
		waitpid(p, &status, 0);
		print_status_info(parsed);
	}

	pid_t pid;  // Terminate all background processes that are in zombie state
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		// fprintf(stdout, "==> terminado: PID=%d", pid);
		//  esto hay que hacerlo con grupos de procesos y señales SIGCHLD
	}

	if (isatty(STDIN_FILENO) && parsed->type == PIPE) {
		set_input_mode();  // To set non canonical mode again after exec pipe
	}

	free_command(parsed);

	return 0;
}