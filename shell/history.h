#include "history_file.h"
#include "defs.h"

typedef struct HistoryState {
	HistoryResult *history_result;
	int history_index;
	char stdout_buffer[BUFLEN];
	int buffer_length;
} HistoryState;

void handle_down(HistoryState *h);
void handle_up(HistoryState *h);
void handle_backspace(HistoryState *h);
void update_stdout_buffer(HistoryState *h);