#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

#include "error.h"
#include "tokenizer.h"

// log.txt does not exist
static int log = -1;

void err_mode(bool mode) {

  if(log == -1) { log = open("log.txt", O_CREAT | O_WRONLY, 0664); }
  
  if(!mode) {
    close(log);
    log = -1;

    pid_t pid = fork();

    if(pid == 0) {
      execlp("rm", "rm", "log.txt", NULL);
    }
    waitpid(pid, NULL, 0);
  }

  dup2(log, STDERR_FILENO);
}

void debug_print_cmds(Command *cmds, size_t count) {
  for (size_t i = 0; i < count; i++) {
    fprintf(stderr, "[CMD %zu]: ", i);
    for (size_t j = 0; j < cmds[i].argc; j++) {
      fprintf(stderr, "'%s' ", cmds[i].argv[j]);
    }
    fprintf(stderr, "\n");
  }
}
