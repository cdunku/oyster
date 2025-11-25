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

void command_free(Command *cmd) {
  if(cmd == NULL) { return; }

  if(cmd->argv != NULL) { vector_free(cmd->argv, cmd->argc); }
  cmd->argc = 0;
  
  free(cmd);
}
