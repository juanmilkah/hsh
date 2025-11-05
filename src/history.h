#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

#define HIST_MAX_LEN 1000

typedef struct {
	size_t start;
	size_t current;
	struct Vec *commands; // List of pointers to commands strings
	const char *path;
	size_t max_len;
} command_history;

// read past commands from disk
command_history *history_init(const char *path);
void history_push(command_history *__restrict__ h, const char *command);
unsigned int history_next_offset(command_history *h);
unsigned int history_prev_offset(command_history *h);
const char *history_next(command_history *h);
const char *history_prev(command_history *h);

#endif
