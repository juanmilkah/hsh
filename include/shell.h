#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>

extern char **environ;

typedef struct {
	bool fatal_error;
	bool is_interactive_mode;
	bool had_error;
	char *name;
	int line_number;
} ShellState;

extern ShellState *shell;

#endif
