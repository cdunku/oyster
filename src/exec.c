#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "exec.h"
#include "tokenizer.h" 
#include "helper.h"
#include "error.h"

#define BUFFER_SIZE 64

BUILT_IN_CMD get_cmd(const char *cmd) {
  if(cmd == NULL) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "cd") == 0) { return CMD_CD; }
  else if(strcmp(cmd, "help") == 0) { return CMD_HELP; }
  else if(strcmp(cmd, "echo") == 0) { return CMD_ECHO; }
  else { return CMD_EXTERNAL; }
}

void redirect_io(Command *cmd, size_t i) {
  Command *cmd_ = &cmd[i];

  if(cmd_->file_in != NULL){
    int fd = open(cmd_->file_in, O_RDONLY);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to read from %s\n", cmd_->file_in);
      return;
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  if(cmd_->file_out != NULL && cmd_->file_err != NULL) {
    int fd; 
    int ff = cmd_->stdio_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) :
                                       (O_WRONLY | O_CREAT | O_TRUNC);
    fd = open(cmd_->file_out, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error unable to write output and error to %s\n", cmd_->file_out);
      return;
    }
    fflush(stdout);
    dup2(fd, STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(fd);
  }
  if(cmd_->file_out != NULL){
    int fd; 
    int ff = cmd_->stdio_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) :
                                       (O_WRONLY | O_CREAT | O_TRUNC);
    fd = open(cmd_->file_out, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to write output to %s\n", cmd_->file_out);
      return;
    }
    fflush(stdout);
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  if(cmd_->file_err != NULL) {
    int fd; 
    int ff = cmd_->stderr_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) : 
                                        (O_WRONLY | O_CREAT | O_TRUNC);
            
    fd = open(cmd->file_err, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to write error to %s\n", cmd_->file_err);
      return;
    }
    fflush(stdout);
    dup2(fd, STDERR_FILENO);
    close(fd);
  }
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

void built_in_cmd(Command *cmd) {
 
  BUILT_IN_CMD current_cmd = get_cmd(cmd->argv[0]);
  
  switch (current_cmd) {
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
      size_t i = 1;
      while(i < cmd->argc) {
        if (i > 1) { // Print space only before the second and subsequent arguments
          
          fprintf(stdout, " ");
        }
        fprintf(stdout, "%s", cmd->argv[i]);
        i++;
      }
      printf("\n");
      fflush(stdout);
      break;
    default:
      fprintf(stderr, "oyster: command not found\n");
      break;
  }
}


void handle_exec(Command *cmd, size_t cmd_count) {

  if(strcmp(cmd->argv[0], "exit") == 0) { return; }

  if(cmd_count > 1) {
    size_t number_of_pipes = cmd_count - 1;

    int pipes[number_of_pipes][2];
    pid_t pids[cmd_count];

    for(size_t i = 0; i < number_of_pipes; i++) {
      if(pipe(pipes[i]) == -1) {
        fprintf(stderr, "Error: pipe failed to initialise\n");
        all_commands_free(cmd, cmd_count);
        return;
      }
    }

    for(size_t i = 0; i < cmd_count; i++) {
      pids[i] = fork();
      if(pids[i] == -1) {
        fprintf(stderr, "Error: failed to create process\n");
        all_commands_free(cmd, cmd_count);
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

        if(cmd->file_in != NULL || cmd->file_out != NULL || cmd->file_err != NULL) { redirect_io(cmd, i); }

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
    BUILT_IN_CMD command = get_cmd(cmd[0].argv[0]);

    if(command == CMD_EXTERNAL && cmd[0].file_in == NULL && cmd[0].file_out == NULL && cmd[0].file_err == NULL) {
      pid_t pid = fork();

      if(pid == -1) {
        fprintf(stderr, "Fatal: child process failed to execute command\n");
        exit(EXIT_FAILURE);
      }
      if(pid == 0) {
        execvp(cmd[0].argv[0], cmd[0].argv);
        fprintf(stderr, "Fatal: unable to execute command\n");
        exit(EXIT_FAILURE);
      }

      waitpid(pid, NULL, 0);
    }
    else if(cmd[0].file_in != NULL || cmd[0].file_out != NULL || cmd[0].file_err != NULL) {
      pid_t pid = fork();
      if(pid == -1) {
        fprintf(stderr, "Fatal: child process failed to execute command\n");
        exit(EXIT_FAILURE);
      }
      if(pid == 0) {
        redirect_io(cmd, 0);
        if(command == CMD_EXTERNAL) {
          debug_print_cmds(cmd, 1);
          execvp(cmd[0].argv[0], cmd[0].argv);
          fprintf(stderr, "Fatal: unable to execute command\n");
          exit(EXIT_FAILURE);
        }
        else {
          debug_print_cmds(cmd, 1);
          built_in_cmd(&cmd[0]);
          exit(EXIT_SUCCESS);
        }
      }
      waitpid(pid, NULL, 0);
    }
    else { 
      debug_print_cmds(cmd, 1);
      built_in_cmd(&cmd[0]);
    }
    return;
  }
}
