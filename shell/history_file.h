#include <stdio.h>

#define HISTORY_ENV_NAME "HISTFILE"
#define HOME_ENV_NAME "HOME"
#define DEFAULT_HISTORY_PATH "/.fisop_history.txt"

typedef struct HistoryResult {
	char **lines;
	size_t size;
} HistoryResult;

void HistoryResult_destroy(HistoryResult *h);

void get_history_file_path(char *buffer);
void write_to_history(const char *cmd);
HistoryResult *read_from_history(FILE *history_file, int n);
