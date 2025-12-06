#include <stdio.h>
#include <stdlib.h>

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
  if(cmd->file_in != NULL) { free(cmd->file_in); }
  if(cmd->file_out != NULL) { free(cmd->file_out); }
  
  free(cmd);
}

// Free mutliple allocations
void all_commands_free(Command *cmd, size_t total_cmds) {
  if(cmd == NULL) { return; }

  for(size_t i = 0; i < total_cmds; i++) {
    if(cmd[i].argv != NULL) {
      for(size_t j = 0; j < cmd[i].argc; j++) {
        free(cmd[i].argv[j]);
      }
      free(cmd[i].argv);
    }
    
    cmd->argc = 0;
    if(cmd[i].file_in != NULL) { free(cmd[i].file_in); }
    if(cmd[i].file_out != NULL) { free(cmd[i].file_out); }
  }  
  free(cmd);
}
