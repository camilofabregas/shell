#include "history.h"
#include <stdio.h>

void
handle_down(HistoryState *h)
{
	if (h->history_index < (int) h->history_result->size - 1) {
		h->history_index++;
		update_stdout_buffer(h);

	} else if (h->history_index == (int) h->history_result->size - 1) {
		// Borra el comando actual cuando llega al final del historial
		h->history_index++;
		while (h->buffer_length > 0) {
			handle_backspace(h);
		}
		memset(h->stdout_buffer, 0, BUFLEN);
		h->buffer_length = 0;
	} else if (h->history_index == (int) h->history_result->size) {
		// Borra el comando actual cuando llega al final del historial
		while (h->buffer_length > 0) {
			handle_backspace(h);
		}
		memset(h->stdout_buffer, 0, BUFLEN);
		h->buffer_length = 0;
	}
}
void
handle_up(HistoryState *h)
{
	if (h->history_index > 0) {
		h->history_index--;
		update_stdout_buffer(h);
	}
}

void
handle_backspace(HistoryState *h)
{
	if (h->buffer_length != 0) {
		printf("%c %c", BACKSPACE, BACKSPACE);
		h->stdout_buffer[--h->buffer_length] = END_STRING;
	}
}

void
update_stdout_buffer(HistoryState *h)
{
	printf("\033[2K\r");

	memset(h->stdout_buffer, 0, BUFLEN);
	strcpy(h->stdout_buffer, h->history_result->lines[h->history_index]);
	h->buffer_length = strlen(h->history_result->lines[h->history_index]);
	h->stdout_buffer[h->buffer_length] = '\0';

	printf("\r$ %s", h->stdout_buffer);
	fflush(stdout);
}