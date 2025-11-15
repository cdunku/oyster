#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#include "exec.h"
#include "helper.h"

#define BUFFER_SIZE 64

BUILT_IN_CMD get_cmd(const char *cmd) {
  if(!cmd) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "cd") == 0) { return CMD_CD; }
  else if(strcmp(cmd, "exit") == 0) { return CMD_EXIT; }
  else if(strcmp(cmd, "help") == 0) { return CMD_HELP; }
  else { return CMD_EXTERNAL; }
}

// Commands in functions

void cmd_cd(char **argv) {
  if(argv[1] == NULL || 
    strcmp(argv[1], "~") == 0) {
    chdir(getenv("HOME"));
    return;   
  } 
  else if(strcmp(argv[1], ".") == 0) {
    chdir(".");
    return;
  }
  else if(strcmp(argv[1], "..") == 0) {
    chdir("..");
    return;
  }
  else {
    chdir(argv[1]);
    return;
  }
} 

void built_in_exec(BUILT_IN_CMD cmd, char **argv, char **envp) {  
  switch (cmd) {
    case CMD_EXIT:
      exit(0);
      
    case CMD_CD:
      cmd_cd(argv);
      break;
    case CMD_PWD:
      // Linux's buffer is usualy 4096 long, while MacOS and BSDs use 1024
      char buffer[1024];
      if(getcwd(buffer, 1024) == NULL) {
        perror("Unable to get PATH");
      }
      else {
        printf("%s\n", buffer);
      }
      break;

    case CMD_ECHO:
      printf("%s", argv);
      break;
    default:
      fprintf(stderr, "oyster: command not found\n");
      break;
  }
}
void external_exec(char **argv, char **envp) { 
  pid_t pid = fork();

  if(pid == -1) { 
    fprintf(stderr, "Error: failed to create process\n");
    return;
  }
  if(pid == 0) {
    if(execvp(argv[0], argv) == -1) {
      perror("Failed to execute command");
      exit(EXIT_FAILURE);
    }
  }
  if(waitpid(pid, NULL, 0) < 0) {
    perror("Child quit status:");
    exit(EXIT_FAILURE);
  }
}

void handle_exec(char **argv, char **envp) {
  if(argv[0] == NULL) { return; }

  BUILT_IN_CMD cmd = get_cmd(argv[0]);
  switch(cmd) {
    case CMD_EXIT:
    case CMD_CD:
      built_in_exec(cmd, argv, NULL);
      break;
    case CMD_EXTERNAL:
      external_exec(argv, NULL);
      break;

    default:
      fprintf(stderr, "oyster: command not found\n");
      break;
  }
  return;
}
