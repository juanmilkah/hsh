#include "history.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HIST_PATH ".hsh_history"

int main(int argc, char **argv)
{
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
		return 127;
	}

	char *name = argc == 1 ? argv[0] : argv[1];
	bool is_interactive = (argc == 1 && isatty(STDIN_FILENO));

	ShellState *shell = shell_init(name, is_interactive, HIST_PATH);

	if (!shell) {
		fprintf(stderr, "ERROR: Shell Init failed\n");
		return 127;
	}

	const char *commands[] = { "Hello", "mike", "angelo" };
	for (int i = 0; i < 3; i++) {
		history_push(shell->history, commands[i]);
	}
	puts("Done push");

	for (int i = 3; i > 0; i--) {
		printf("%s\n", history_prev(shell->history));
	}
	exit(0);

	if (argc == 2) {
		FILE *stream = fopen(argv[1], "r");
		if (!stream) {
			fprintf(stderr, "Error: cannot open file %s\n",
				argv[1]);
			shell_free(shell);
			return 127;
		}
		shell_repl(shell, stream);
		fclose(stream);
	} else {
		shell_repl(shell, stdin);
	}

	int exit_code = shell->fatal_error ? 2 : 0;
	shell_free(shell);
	return exit_code;
}
