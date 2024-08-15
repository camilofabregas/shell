#include "exec.h"

#define STDERR_TO_STDOUT_VALUE "&1"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	int equals_idx;
	for (int i = 0; i < eargc; i++) {
		equals_idx = block_contains(eargv[i], '=');
		char environ_key[equals_idx];
		char environ_value[strlen(eargv[i]) - equals_idx];
		get_environ_key(eargv[i], environ_key);
		get_environ_value(eargv[i], environ_value, equals_idx);
		if (setenv(environ_key, environ_value, 1) < 0) {
			perror("Error setting temporary environment variable");
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int flags_to_use = 0;

	// Checks if the flag O_CREAT is set
	if ((flags & O_CREAT) == O_CREAT) {
		flags_to_use = S_IWUSR | S_IRUSR;
	}

	int fd = open(file, flags | O_CLOEXEC, flags_to_use);

	return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);

		int status = execvp(e->argv[0], e->argv);
		if (status < 0) {
			perror("Error running command");
			_exit(-1);
		}
		break;


	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		int p_fork = fork();
		switch (p_fork) {
		case -1: {
			fprintf(stderr,
			        "Error creating fork for command %s",
			        r->argv[0]);
			_exit(-1);
			break;
		}
		case 0: {
			// Redirect stdin if in_file exists
			if (strlen(r->in_file) != 0) {
				int fd_in = open_redir_fd(r->in_file, O_RDONLY);
				if (fd_in < 0) {
					fprintf(stderr,
					        "Error opening file %s",
					        r->in_file);
					_exit(-1);
				}

				dup2(fd_in, STDIN_FILENO);
				close(fd_in);
			}

			// Redirect stdout to file if out_file exists
			if (strlen(r->out_file) != 0) {
				int fd_out = open_redir_fd(r->out_file,
				                           O_WRONLY | O_CREAT |
				                                   O_TRUNC);
				if (fd_out < 0) {
					fprintf(stderr,
					        "Error opening file %s",
					        r->out_file);
					_exit(-1);
				}

				dup2(fd_out, STDOUT_FILENO);

				// Handle case of 2>&1 (Stderr and stdout to same output)
				if (strlen(r->err_file) != 0 &&
				    strcmp(r->err_file,
				           STDERR_TO_STDOUT_VALUE) == 0) {
					dup2(fd_out, STDERR_FILENO);
				}

				close(fd_out);
			}

			// Redirect stderr to file if err_file exists
			if (strlen(r->err_file) != 0 &&
			    strcmp(r->err_file, STDERR_TO_STDOUT_VALUE) != 0) {
				int fd_err = open_redir_fd(r->err_file,
				                           O_WRONLY | O_CREAT |
				                                   O_TRUNC);

				if (fd_err < 0) {
					fprintf(stderr,
					        "Error opening file %s",
					        r->err_file);
					_exit(-1);
				}

				dup2(fd_err, STDERR_FILENO);
				close(fd_err);
			}

			r->type = EXEC;
			exec_cmd((struct cmd *) r);
			break;
		}
		}
		wait(NULL);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;

		int fd[2];
		if (pipe(fd) == -1) {
			perror("Error creating pipe\n");
			_exit(-1);
		}

		int f1 = fork();
		if (f1 == 0) {
			close(fd[READ]);
			dup2(fd[WRITE], 1);
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		} else {
			int f2 = fork();
			if (f2 == 0) {
				close(fd[WRITE]);
				dup2(fd[READ], 0);
				close(fd[READ]);
				exec_cmd(p->rightcmd);
			}
			wait(&f2);
		}
		close(fd[WRITE]);
		close(fd[READ]);
		wait(&f1);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		exit(0);

		break;
	}
	}
}