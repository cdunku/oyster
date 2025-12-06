#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#include "exec.h"
#include "tokenizer.h" 
#include "helper.h"

#define BUFFER_SIZE 64

BUILT_IN_CMD get_cmd(const char *cmd) {
  if(cmd == NULL) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "cd") == 0) { return CMD_CD; }
  else if(strcmp(cmd, "exit") == 0) { return CMD_EXIT; }
  else if(strcmp(cmd, "help") == 0) { return CMD_HELP; }
  else { return CMD_EXTERNAL; }
}

BUILT_IN_CMD get_redirector(const char *cmd) {
  if(cmd == NULL) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "|") == 0) { return CMD_PIPE_REDIRECT; }
  else if(strcmp(cmd, ">") == 0) { return CMD_OUTPUT_REDIRECT; }
  else if(strcmp(cmd, "<") == 0) { return CMD_INPUT_REDIRECT; }
  else if(strcmp(cmd, ">>") == 0) { return CMD_APPEND_REDIRECT; }
  else if(strcmp(cmd, "2>") == 0) { return CMD_ERROR_REDIRECT; }
  else if(strcmp(cmd, "&>") == 0) { return CMD_BOTH_REDIRECT; }
  else { return CMD_IS_COMMAND; }
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

void external_exec(char **argv) {
  pid_t pid = fork();

  if(pid == -1) {
    return;
  }

  if(pid == 0) { execvp(argv[0], argv); }

  waitpid(pid, NULL, 0);
}



void handle_exec(Command *cmd, size_t cmd_count) {

  if(cmd_count > 1) {
    size_t number_of_pipes = cmd_count - 1;

    int pipes[number_of_pipes][2];
    pid_t pids[cmd_count];

    for(size_t i = 0; i < number_of_pipes; i++) {
      if(pipe(pipes[i]) == -1) {
        fprintf(stderr, "Error: pipe failed to initialise\n");
        return;
      }
    }

    for(size_t i = 0; i < cmd_count; i++) {
      pids[i] = fork();
      if(pids[i] == -1) {
        fprintf(stderr, "Error: failed to create process\n");
        return;
      }

      if(pids[i] == 0) {

        if (i > 0) {
          dup2(pipes[i - 1][0], STDIN_FILENO);
        }
        if (i < cmd_count - 1) {
          dup2(pipes[i][1], STDOUT_FILENO);
        }

        for(size_t j = 0; j < number_of_pipes; j++) {    
          close(pipes[j][0]);         
          close(pipes[j][1]);   
        }

        if(cmd[i].file_in != NULL) {
          freopen(cmd[i].file_in, "r", stdin);
        }
        if(cmd[i].file_out != NULL) {
          freopen(cmd[i].file_out, "w", stdout);
        }

        if(execvp(cmd[i].argv[0], cmd[i].argv) == -1) {
          fprintf(stderr, "Fatal: child process failed to execute command\n");
          exit(EXIT_FAILURE);
        };
      }
    }
    for(size_t j = 0; j < number_of_pipes; j++) {    
      close(pipes[j][0]); 
      close(pipes[j][1]); 
    }

    for(size_t j = 0; j < cmd_count; j++) { waitpid(pids[j], NULL, 0); }
  }
  else {
    BUILT_IN_CMD current_cmd = get_cmd(cmd->argv[0]);
    switch (current_cmd) {
      case CMD_EXIT:
        exit(0);
        
      case CMD_CD:
        cmd_cd(cmd->argv);
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
        printf("%s", cmd->argv);
        break;

      case CMD_EXTERNAL:
        external_exec(cmd->argv);
        break;
      default:
        fprintf(stderr, "oyster: command not found\n");
        break;
    }
  }
}
