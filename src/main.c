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

  Pipeline p = { .cmd = NULL, .argv_size = 0, .cmd_count = 0 }; // Initialize to NULL/zero
  
  while (1) {
    // Clean previous iteration
    if (str != NULL) { free(str); str = NULL; }
    if(p.cmd != NULL) {
      if (p.cmd->argv != NULL) {
        vector_free(p.cmd->argv, p.argv_size);
        p.cmd->argv = NULL;
      }
      if (p.cmd->input != NULL) {
        // free(p.cmd->input);
        p.cmd->input = NULL;
      }
      if (p.cmd->output != NULL) {
        // free(p.cmd->output);
        p.cmd->output = NULL;
      }
    }

    free(p.cmd);
    p.cmd = NULL;

    if (write(STDOUT_FILENO, "$ ", 2) == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }

    str = getl();  // must return malloc’d string or NULL
    if (!str) continue;   // or break if you prefer EOF exit

    p = tokenizer(str);  // tokenizer must allocate p.cmd internally

    if (!p.cmd || !p.cmd->argv) continue;

    handle_exec(p);

    if (strncmp(str, "quit", 4) == 0) {
      fprintf(stdout, "Exited oyster shell\n");
      if(p.cmd != NULL) {
        vector_free(p.cmd->argv, p.argv_size);
        free(p.cmd);
      }

      free(str);
      break;
    }
  }

  return 0;
}
