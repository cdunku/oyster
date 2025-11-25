#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "tokenizer.h"
#include "exec.h"
#include "helper.h"


int main(void) {
  char *str = NULL;

  while (1) {
    if (write(STDOUT_FILENO, "$ ", 2) == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }

    str = getl();            
    if (!str) continue;      

    Token *tokens = tokenizer(str);
    if (!tokens) {
      free(str);
      continue;
    }

    Command *cmd = parse_cmds(tokens);
    if (!cmd) {
      token_list_free(tokens);
      free(str);
      continue;
    }

    handle_exec(cmd->argv, NULL);

    if (strncmp(str, "quit", 4) == 0) {
      fprintf(stdout, "Exited oyster shell\n");
      token_list_free(tokens);
      command_free(cmd);
      free(str);
      return 0;
    }

    token_list_free(tokens);   
    command_free(cmd);         
    free(str);
    str = NULL;
  }

  return 0;
}
