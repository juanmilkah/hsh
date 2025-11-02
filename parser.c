#include <stdbool.h>
#include <tokenizer.h>
#include <utils.h>
#include <shell.h>
#include <parser.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static Token *current, *prev;

static Token *previous(void)
{
	return prev;
}

static Token *peek(void)
{
	return current;
}

static int isAtEnd(void)
{
	return peek()->type == TOKEN_EOL;
}

static Token *advance(void)
{
	Token *token = peek();
	if (!isAtEnd())
		current = current->next;
	return token;
}

static bool match(int n, ...)
{
	if (isAtEnd())
		return false;
	va_list args;
	va_start(args, n);
	for (int i = 0; i < n; i++) {
		TokenType type = va_arg(args, TokenType);
		if (peek()->type == type) {
			prev = advance();
			va_end(args);
			return true;
		}
	}
	va_end(args);
	return false;
}

static Command *simple_command(void)
{
	SimpleCommand *simple = malloc(sizeof(SimpleCommand));
	int envc = 0, capacity = 2;

	if (!simple) {
		shell->fatal_error = true;
		return NULL;
	}
	simple->argc = 0;
	simple->argv = malloc(sizeof(char *) * capacity);
	simple->envp = malloc(sizeof(char *) * capacity);
	simple->input_file = NULL;
	simple->output_file = NULL;
	simple->append_output = false;
	if (!simple->argv || !simple->envp) {
		shell->fatal_error = true;
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}
	while (match(1, TOKEN_ASSIGNMENT_WORD)) {
		if (envc + 1 >= capacity) {
			capacity *= 2;
			char **new_envp = realloc(simple->envp,
						  capacity * sizeof(char *));
			if (!new_envp) {
				shell->fatal_error = true;
				free(simple->envp);
				free(simple->argv);
				free(simple);
				return NULL;
			}
			simple->envp = new_envp;
		}
		simple->envp[envc++] = previous()->lexeme;
	}
	simple->envp[envc] = NULL;
	capacity = 2;
	if (match(1, TOKEN_WORD)) {
		if (simple->argc + 1 >= capacity) {
			capacity *= 2;
			char **new_argv = realloc(simple->argv,
						  sizeof(char *) * capacity);
			if (!new_argv) {
				shell->fatal_error = true;
				free(simple->argv);
				free(simple->envp);
				free(simple);
				return NULL;
			}
			simple->argv = new_argv;
			simple->argv[simple->argc++] = previous()->lexeme;
		}
		simple->argv[simple->argc++] = previous()->lexeme;
		simple->argv[simple->argc] = NULL;
	} else if (isAtEnd()) {
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	} else {
		shell->had_error = true;
		printf("%s: %d: Syntax error: \"%s\" unexpected\n", shell->name,
		       shell->line_number,
		       (previous() ? previous() : peek())->lexeme);
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}
	while (true) {
		if (match(2, TOKEN_WORD, TOKEN_ASSIGNMENT_WORD)) {
			if (simple->argc + 1 >= capacity) {
				capacity *= 2;
				char **new_argv =
					realloc(simple->argv,
						sizeof(char *) * capacity);
				if (!new_argv) {
					shell->fatal_error = true;
					free(simple->argv);
					free(simple->envp);
					free(simple);
					return NULL;
				}
				simple->argv = new_argv;
			}
			simple->argv[simple->argc++] = previous()->lexeme;
			simple->argv[simple->argc] = NULL;

		} else if (match(3, TOKEN_REDIRECT_IN, TOKEN_REDIRECT_OUT,
				 TOKEN_REDIRECT_APPEND)) {
			Token *op = previous();

			if (!match(1, TOKEN_WORD)) {
				fprintf(stderr,
					"%s: %d: Syntax error: "
					"expected filename after '%s'\n",
					shell->name, shell->line_number,
					op->lexeme);
				free(simple->argv);
				free(simple->envp);
				free(simple);
				return NULL;
			}

			Token *filename = previous();

			switch (op->type) {
			case TOKEN_REDIRECT_IN:
				simple->input_file = filename->lexeme;
				break;
			case TOKEN_REDIRECT_OUT:
				simple->output_file = filename->lexeme;
				simple->append_output = false;
				break;
			case TOKEN_REDIRECT_APPEND:
				simple->output_file = filename->lexeme;
				simple->append_output = true;
				break;
			default:
				break;
			}
		} else {
			break;
		}
	}
	Command *cmd = malloc(sizeof(Command));
	if (!cmd) {
		shell->fatal_error = true;
		free(simple->argv);
		free(simple->envp);
		free(simple);
		return NULL;
	}
	cmd->type = CMD_SIMPLE;
	cmd->as.command = *simple;
	cmd->is_background = false;
	free(simple);
	return cmd;
}

static Command *pipeline(void)
{
	Command *cmd = simple_command();
	if (cmd)
		cmd->is_background = false;
	else if (shell->had_error)
		return NULL;
	while (match(1, TOKEN_PIPE)) {
		if (isAtEnd()) {
			shell->had_error = true;
			printf("%s: %d: Syntax error: end of line unexpected\n",
			       shell->name, shell->line_number);
			free_command(cmd);
			return NULL;
		}
		Command *right = simple_command();
		if (!right) {
			free_command(cmd);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		if (!parent) {
			free_command(cmd);
			free_command(right);
			return NULL;
		}
		parent->as.binary.left = cmd;
		parent->type = CMD_PIPE;
		parent->as.binary.right = right;
		cmd = parent;
		cmd->is_background = false;
	}
	return cmd;
}

static Command *logical_list(void)
{
	Command *cmd = pipeline();
	if (cmd)
		cmd->is_background = false;
	else if (shell->had_error)
		return NULL;
	while (match(2, TOKEN_AND, TOKEN_OR)) {
		if (isAtEnd()) {
			shell->had_error = true;
			printf("%s: %d: Syntax error: end of line unexpected\n",
			       shell->name, shell->line_number);
			free_command(cmd);
			return NULL;
		}
		CommandType parent_type =
			previous()->type == TOKEN_AND ? CMD_AND : CMD_OR;
		Command *right = pipeline();
		if (!right) {
			free_command(cmd);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		parent->type = parent_type;
		if (!parent) {
			free_command(cmd);
			free_command(right);
			return NULL;
		}
		parent->as.binary.left = cmd;
		parent->as.binary.right = right;
		cmd = parent;
		cmd->is_background = false;
	}
	return cmd;
}

static Command *command(void)
{
	Command *cmd = logical_list();
	if (shell->had_error) {
		// free_command(cmd);
		return NULL;
	}
	while (match(1, TOKEN_BACKGROUND)) {
		cmd->is_background = true;
		if (shell->had_error) {
			free_command(cmd);
			return NULL;
		}
		if (isAtEnd())
			return cmd;
		Command *right = logical_list();
		if (shell->had_error || !right) {
			free_command(cmd);
			free_command(right);
			return NULL;
		}
		Command *parent = malloc(sizeof(Command));
		if (shell->had_error || !parent) {
			free_command(parent);
			free_command(cmd);
			free_command(right);
			return NULL;
		}
		parent->as.binary.left = cmd;
		parent->type = CMD_SEPARATOR;
		parent->as.binary.right = right;
		cmd = parent;
	}
	return cmd;
}

/**
 * parse - parses the tokens into a command structure
 * @tokens: the list of tokens
 * Return: pointer to the command structure or NULL on failure
 */
Command *parse(Token *tokens)
{
	current = tokens;
	prev = NULL;
	return command();
}
