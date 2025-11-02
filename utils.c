#include <utils.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

void freevec(char **vector)
{
	char **tmp = vector;

	if (vector == NULL)
		return;
	while (*vector) {
		free(*vector);
		vector++;
	}
	free(tmp);
}

void free_tokens(Token *head)
{
	Token *current = head;
	Token *next;

	while (current != NULL) {
		next = current->next;
		if (current->type == TOKEN_WORD ||
		    current->type == TOKEN_ASSIGNMENT_WORD)
			free(current->lexeme);
		free(current);
		current = next;
	}
}

void free_command(Command *command)
{
	if (command == NULL)
		return;

	if (command->type == CMD_SIMPLE) {
		free(command->as.command.argv);
		free(command->as.command.envp);
	} else {
		free_command(command->as.binary.left);
		free_command(command->as.binary.right);
	}
	free(command);
}
