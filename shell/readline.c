#include "defs.h"
#include "readline.h"
#include "history.h"
#include <ctype.h>

char *read_line_canonical_mode(void);
char *read_line_non_canonical_mode(void);

static HistoryState history_state = { NULL, 0, "", 0 };

static char buffer[BUFLEN];

// reads a line from the standard input
// and prints the prompt
char *
read_line(const char *prompt)
{
#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	if (!isatty(STDIN_FILENO)) {
		return read_line_canonical_mode();
	} else {
		return read_line_non_canonical_mode();
	}
}

char *
read_line_canonical_mode()
{
	int i = 0, c = 0;

	memset(buffer, 0, BUFLEN);

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

char *
read_line_non_canonical_mode()
{
	char path_history[BUFLEN];
	get_history_file_path(path_history);

	FILE *history = fopen(path_history, "r");
	if (!history) {
		// Create file if it does't exists
		history = fopen(path_history, "w");
		if (history == NULL) {
			printf("Error opening history file");
			exit(-1);
		}
	}

	HistoryResult *history_result = read_from_history(history, 0);
	fclose(history);

	history_state.history_result = history_result;
	history_state.history_index = history_result->size;
	history_state.buffer_length = 0;
	int c = 0;

	memset(history_state.stdout_buffer, 0, BUFLEN);

	c = getchar();

	while (c != END_LINE) {
		if (c == CTRL_D) {  // CTRL + D
			printf("%c", END_LINE);
			return NULL;  // To exit shell normally
		}
		if (c == DELETE) {  // Backspace
			handle_backspace(&history_state);
		}

		if (c == ESCAPE) {  // Escape character
			char esc_seq;
			esc_seq = getchar();
			if (esc_seq == '[') {
				esc_seq = getchar();
				switch (esc_seq) {
				case 'A': {  // Up arrow
					handle_up(&history_state);
					break;
				}
				case 'B': {  // Down arrow
					handle_down(&history_state);
					break;
				}
				case 'C': {  // Right arrow
					printf("\033[1C");
					break;
				}
				case 'D': {  // Left arrow
					printf("%c", BACKSPACE);
					break;
				}
				}
			}
		}
		if (isprint(c)) {  // Show visible characters
			printf("%c", c);
			history_state.stdout_buffer[history_state.buffer_length++] =
			        c;
		}
		c = getchar();
	}

	HistoryResult_destroy(history_result);
	history_state.history_result = NULL;
	history_state.stdout_buffer[history_state.buffer_length] =
	        END_STRING;  // Enter
	printf("%c", c);
	return history_state.stdout_buffer;
}
