#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <stdio.h>
#include "history.h"

typedef struct ShellState {
	bool fatal_error;
	bool is_interactive_mode;
	bool had_error;
	char *name;
	int line_number;
	command_history *history;
} ShellState;

ShellState *shell_init(char *name, bool is_interactive, const char *hist_path);
void shell_free(ShellState *shell);
void shell_repl(ShellState *shell, FILE *stream);

#endif
