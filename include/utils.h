#ifndef UTILS_H
#define UTILS_H

#include <tokenizer.h>
#include <parser.h>

void freevec(char **vector);

void free_tokens(Token *head);

void free_command(Command *command);

#endif
