#include "defs.h"
#include "history_file.h"

void
HistoryResult_destroy(HistoryResult *h)
{
	for (size_t i = 0; i < h->size; i++) {
		free(h->lines[i]);
	}


	free(h->lines);
	free(h);
}

void
get_history_file_path(char *buffer)
{
	if (getenv(HISTORY_ENV_NAME)) {
		strcpy(buffer, getenv(HISTORY_ENV_NAME));
	} else {
		strcpy(buffer, getenv(HOME_ENV_NAME));
		strcat(buffer, DEFAULT_HISTORY_PATH);
	}
}


void
write_to_history(const char *cmd)
{
	char history_path[BUFLEN];
	get_history_file_path(history_path);


	FILE *history_file = fopen(history_path, "a+");
	if (history_file == NULL) {
		perror("Error opening history file\n");
		exit(-1);
	}


	fprintf(history_file, "%s\n", cmd);
	fclose(history_file);
}

HistoryResult *
read_from_history(FILE *history_file, int n)
{
	int line_count = 0;
	int c;
	while ((c = fgetc(history_file)) != EOF) {
		if (c == '\n') {
			line_count++;
		}
	}

	// Rewind file pointer to the beginning
	rewind(history_file);

	// If n bigger than total of lines or equal to 0
	// Get all lines from the file
	if (n > line_count || n == 0) {
		n = line_count;
	}

	// Allocate memory for the lines array
	char **lines = malloc(sizeof(char *) * n);
	if (lines == NULL) {
		perror("Error allocating memory\n");
		return NULL;
	}

	// Read the lines from the file
	int i, j;
	size_t len;
	char *line = NULL;
	ssize_t n_read;
	for (i = 0; i < line_count - n; i++) {
		// Skip lines before the desired ones
		n_read = getline(&line, &len, history_file);
		if (n_read < 0) {
			perror("Error while reading line with getline");
		}
	}

	for (j = 0; j < n && getline(&line, &len, history_file) != -1; j++) {
		// Remove the newline character from the end of the line
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';
		}

		// Allocate memory for the line
		lines[j] = malloc(sizeof(char) * (strlen(line) + 1));
		if (lines[j] == NULL) {
			perror("Error allocating memory\n");
			return NULL;
		}

		// Copy the line into the array
		strcpy(lines[j], line);
	}
	// Free memory and close the file
	free(line);

	HistoryResult *result = malloc(sizeof(HistoryResult));
	result->lines = lines;
	result->size = n;


	return result;
}