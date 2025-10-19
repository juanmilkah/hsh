#include <shell.h>
#include <sys/types.h>
#include <utils.h>
#include <stdlib.h>
#include <stdio.h>
#include <tokenizer.h>
#include <parser.h>

int run(char *source)
{
	const Token *tokens;
	Command *command;

	tokens = tokenize(source);
	if (!tokens)
		return (1);
	command = parse(tokens);
	if (!command)
		return (1);
	while (tokens && tokens->type != TOKEN_EOF) {
		if (tokens->type == TOKEN_NEWLINE &&
		    tokens->next->type != TOKEN_EOF) {
			command = parse(tokens->next);
		}
	}
	return (0);
}

/**
 * repl - Read-Eval-Print Loop for interactive shell
 * Return: exit status
 */
static int repl(void)
{
	char *line = NULL;
	size_t n;
	int exit_value = 0;
	ssize_t nread = 0;

	for (;;) {
		if (nread < 0 || exit_value != 0) {
			free(line);
			break;
		}
		nread = getline(&line, &n, stdin);
		exit_value = run(line);
		line = NULL;
	}
	return exit_value;
}
/**
 * loop - main loop to run the shell
 * @source: source code to execute, NULL for REPL
 * Return: exit status
 */
int loop(char *source)
{
	return (source ? run(source) : repl());
}
