#pragma once

#include "tokenizer.h"

void vector_free(char **argv, size_t i);
void token_list_free(Token *t);
void command_free(Command *cmd);
void all_commands_free(Command *cmd, size_t total_cmds);
