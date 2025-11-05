#include <stdlib.h>
#include "history.h"
#define JUAN_IMPLEMENTATION
#include "juan.h"

command_history *history_init(const char *path)
{
	if (create_file_if_not_exists(path) < 0) {
		J_log(J_ERROR, "creat history file");
		return NULL;
	}

	long file_size = size_of_file(path);
	if (file_size < 0) {
		J_log(J_ERROR, "get history file size");
		return NULL;
	}

	struct Vec *array = init_vec_with_cap(HIST_MAX_LEN);
	if (!array)
		return NULL;

	if (file_size > 0) {
		void *buf = malloc(file_size + 1);

		if (read_file_to_buffer(path, buf, file_size + 1) < 0) {
			J_log(J_ERROR, "read history to buf");
			return NULL;
		}

		split_newline_to_vec(buf, array);
	}

	command_history *h = malloc(sizeof(command_history));
	h->commands = array;
	h->path = path;
	h->start = 0;
	h->current = (array->len == 0) ? 0 : array->len--;
	h->max_len = HIST_MAX_LEN;

	return h;
}

void history_push(command_history *__restrict__ h, const char *command)
{
	if (h->commands->len == h->max_len) {
		vec_insert_at(h->commands, (void *)command, h->start);
		h->current = h->start;
		h->start++;

	} else {
		vec_push(h->commands, (void *)command);
		h->current++;
	}
}

unsigned int history_next_offset(command_history *h)
{
	if (!h)
		return -1;
	unsigned int offset = h->current % h->commands->len;
	h->current++;
	return offset;
}

unsigned int history_prev_offset(command_history *h)
{
	if (!h)
		return -1;
	unsigned int offset = h->current % h->commands->len;
	h->current--;
	return offset;
}

const char *history_next(command_history *h)
{
	return h->commands->items[history_next_offset(h)];
}

const char *history_prev(command_history *h)
{
	return h->commands->items[history_prev_offset(h)];
}
