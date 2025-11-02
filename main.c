#include <parser.h>
#include <stdbool.h>
#include <tokenizer.h>
#include <shell.h>
#include <utils.h>
#include <unistd.h>
#include <stdio.h>
#include <executor.h>
#include <stdlib.h>

ShellState *shell;

static Token **rip_off_semicolons(Token *tokens)
{
	Token **commands;
	Token *current = tokens, *prev = NULL, *next = NULL, *start = tokens,
	      *node = malloc(sizeof(Token));
	size_t count = 0, index = 0;

	if (!node) {
		shell->fatal_error = true;
		return NULL;
	}
	count++;
	while (current) {
		if (current->type == TOKEN_SEMICOLON)
			count++;
		current = current->next;
	}
	commands = malloc(sizeof(Token *) * (count + 1));
	if (!commands) {
		shell->fatal_error = true;
		free(node);
		return NULL;
	}

	current = tokens;
	start = current;
	while (current) {
		next = current->next;
		if (current->type == TOKEN_SEMICOLON) {
			node->type = TOKEN_EOL;
			node->lexeme = "\n";
			node->next = NULL;
			if (!prev) {
				commands[index++] = node;
			} else {
				prev->next = node;
				commands[index++] = start;
			}
			start = next;
			node = malloc(sizeof(Token));
			if (!node) {
				shell->fatal_error = true;
				commands[index] = NULL;
				return commands;
			}
		}
		prev = current->type == TOKEN_SEMICOLON ? NULL : current;
		if (current->type == TOKEN_SEMICOLON)
			free(current);
		current = next;
	}
	node->type = TOKEN_EOL;
	node->lexeme = "\n";
	node->next = NULL;
	if (!prev) {
		commands[index++] = node;
	} else {
		prev->next = node;
		commands[index++] = start;
	}
	commands[index] = NULL;
	return commands;
}

static void repl(FILE *stream)
{
	char *line = NULL;
	size_t n = 0;
	ssize_t nread = 0;

	while (true) {
start:
		shell->line_number++;
		if (shell->is_interactive_mode)
			fprintf(stdout, "$ ");
		nread = getline(&line, &n, stream);
		if (nread < 0) {
			free(line);
			break;
		}
		Token *t = tokenize(line);
#ifdef DEBUG
		static char *token_string[] = {
			[TOKEN_ASSIGNMENT_WORD] = "TOKEN_ASSIGNMENT_WORD",
			[TOKEN_WORD] = "TOKEN_WORD",
			[TOKEN_SEMICOLON] = "TOKEN_SEMICOLON",
			[TOKEN_REDIRECT_IN] = "TOKEN_REDIRECT_IN",
			[TOKEN_REDIRECT_OUT] = "TOKEN_REDIRECT_OUT",
			[TOKEN_REDIRECT_APPEND] = "TOKEN_REDIRECT_APPEND",
			[TOKEN_BACKGROUND] = "TOKEN_BACKGROUND",
			[TOKEN_PIPE] = "TOKEN_PIPE",
			[TOKEN_AND] = "TOKEN_AND",
			[TOKEN_OR] = "TOKEN_OR",
			[TOKEN_EOL] = "TOKEN_EOL"
		};
		Token *current = t;
		while (current != NULL) {
			// For debugging: print token types and lexemes
			fprintf(stdout, "[%s: '%s']\n",
				token_string[current->type], current->lexeme);
			current = current->next;
		}
#endif
		free(line);
		line = NULL;
		if (shell->fatal_error) {
			free_tokens(t);
			return;
		} else if (shell->had_error) {
			free_tokens(t);
			shell->had_error = false;
			continue;
		}
		Token **tokens = rip_off_semicolons(t);
		if (shell->fatal_error) {
			Token **runner = tokens;
			while (*runner) {
				free_tokens(*runner);
				runner++;
			}
			free(tokens);
			return;
		}
		Token **ptr = tokens;
		for (; *ptr; ptr++) {
			Command *command = parse(*ptr);
			execute(command);
			free_command(command);
			if (shell->fatal_error) {
				while (*ptr) {
					free_tokens(*ptr);
					ptr++;
				}
				free(tokens);
				return;
			} else if (shell->had_error) {
				shell->had_error = false;
				while (*ptr) {
					free_tokens(*ptr);
					ptr++;
				}
				free(tokens);
				if (shell->is_interactive_mode)
					goto start;
				return;
			}
			free_tokens(*ptr);
		}
		free(tokens);
	}
	if (shell->is_interactive_mode)
		putchar('\n');
}

int main(int argc, char **argv)
{
	shell = malloc(sizeof(ShellState));
	if (!shell) {
		fprintf(stderr, "Error: malloc failed\n");
		return (127);
	}

	shell->fatal_error = false;
	shell->had_error = false;
	shell->is_interactive_mode = true;
	shell->line_number = 0;
	shell->name = argc == 1 ? argv[0] : argv[1];

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [filename]", argv[0]);
		return (127);
	} else if (argc == 2) {
		FILE *stream = fopen(argv[1], "r");
		shell->is_interactive_mode = false;
		if (!stream) {
			fprintf(stderr, "Error: cannot open file %s\n",
				argv[1]);
			return (127);
		}
		repl(stream);
		fclose(stream);
	} else if (!isatty(STDIN_FILENO)) {
		shell->is_interactive_mode = false;
		repl(stdin);
	} else {
		repl(stdin);
	}
	if (shell->fatal_error)
		return (2);
	free(shell);
	return (0);
}
