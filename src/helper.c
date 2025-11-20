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

void error_flush_token(Pipeline* const p, char* token, const char *err_msg) {

  if(p && p->cmd && p->cmd->argv) {
    vector_free(p->cmd->argv, p->argv_size);
    p->cmd->argv = NULL;
  }
  
  if(token != NULL) { free(token); }
  
  fprintf(stderr, "%s\n", err_msg);
  exit(EXIT_FAILURE);
}
