#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helper.h"

void vector_free(char **argv, size_t size) {
  for(size_t i = 0; i < size; i++) {
    free(argv[i]);
  }
  free(argv);
  return;
}

void token_list_free(Token *t) {
  Token *tmp;
  while(t != NULL) {
    tmp = t->next;
    free(t->content);
    free(t);
    t = tmp;
  }
}

// Free one allocation
void command_free(Command *cmd) {
  if(cmd == NULL) { return; }

  if(cmd->argv != NULL) { vector_free(cmd->argv, cmd->argc); }
  cmd->argc = 0;
  if(cmd->stream.file_in != NULL) { free(cmd->stream.file_in); }
  if(cmd->stream.file_out != NULL) { free(cmd->stream.file_out); }
  if(cmd->stream.file_err != NULL) { free(cmd->stream.file_err); }

  cmd->stream.stdio_append = false;
  cmd->stream.stderr_append = false;

  free(cmd);
}


void all_commands_free(Command *cmd, size_t total_cmds) {
  if (cmd == NULL) { return; }

  for (size_t i = 0; i < total_cmds; i++) {
    if (cmd[i].argv) {
      for (size_t j = 0; j < cmd[i].argc; j++) {
        free(cmd[i].argv[j]);
      }
      free(cmd[i].argv);
    }
    if(cmd[i].str_type != NULL) { free(cmd[i].str_type); }

    if(cmd[i].stream.file_in != NULL) { free(cmd[i].stream.file_in); }
    if(cmd[i].stream.file_out != NULL) { free(cmd[i].stream.file_out); }
    if(cmd[i].stream.file_err != NULL) { free(cmd[i].stream.file_err); }

    cmd[i].stream.stdio_append = false;
    cmd[i].stream.stderr_append = false;
  }
  free(cmd); // free array of commands
}

void free_units(ExecutionUnit* units, size_t units_count) {
  for(size_t i = 0; i < units_count; i++) {
    all_commands_free(units[i].pl.cmds, units[i].pl.cmds_count);
  }
  free(units);
}

// Operators for returning the valid operator.
OperatorType get_operator(const char *str) {
  if(strcmp(str, ">")  == 0) { return OP_REDIRECT_OUT; }
  else if(strcmp(str, ">>")  == 0) { return OP_REDIRECT_OUT_APPEND; }
  else if(strcmp(str, "2>") == 0) { return OP_REDIRECT_ERR; } 
  else if(strcmp(str, "2>>") == 0) { return OP_REDIRECT_ERR_APPEND; }
  else if(strcmp(str, "&>") == 0) { return OP_REDIRECT_ALL; } 
  else if(strcmp(str, "&>>") == 0) { return OP_REDIRECT_ALL_APPEND; }
  else if(strcmp(str, ">") ==  0) { return OP_REDIRECT_IN; }
  else if(strcmp(str, "|") == 0) { return OP_PIPE; }
  else if(strcmp(str, "&&") == 0) { return OP_CONDITIONAL_AND; }
  else if(strcmp(str, "||") == 0) { return OP_CONDITIONAL_OR; }
  return OP_NONE;
}
