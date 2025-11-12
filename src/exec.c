#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#include "exec.h"
#include "helper.h"

#define BUFFER_SIZE 64

CMD_TOOLS get_cmd(const char *cmd) {
  if(!cmd) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "cd") == 0) { return CMD_CD; }
  else if(strcmp(cmd, "exit") == 0) { return CMD_EXIT; }

  else if(strcmp(cmd, "ls") == 0) { return CMD_LS; }
  else if(strcmp(cmd, "grep") == 0) { return CMD_GREP; }

  else { return CMD_UNKNOWN; }
}

void built_in_exec(char **argv, char **envp) { return; }
void external_exec(char **argv, char **envp) { return; }

void handle_exec(char **argv, char **envp) {
  if(argv[0] == NULL) { return; }

  switch(get_cmd(argv[0])) {
    case CMD_EXIT:
    case CMD_CD:
      built_in_exec(argv, NULL);
      break;
    
    case CMD_GREP:
    case CMD_LS:
      external_exec(argv, NULL);
      break;

    case CMD_UNKNOWN:
    default:
      fprintf(stderr, "oyster: command not found\n");
      break;
  }
  return;
}
