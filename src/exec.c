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

// Default value is 0, global status of last command.
static int g_last_status = 0;

BUILT_IN_CMD get_cmd(const char *cmd) {
  if(cmd == NULL) { return CMD_UNKNOWN; }

  if(strcmp(cmd, "cd") == 0) { return CMD_CD; }
  else if(strcmp(cmd, "help") == 0) { return CMD_HELP; }
  else if(strcmp(cmd, "echo") == 0) { return CMD_ECHO; }
  else { return CMD_EXTERNAL; }
}

// Commands in functions

int cmd_cd(char **argv) {

  const char* directory;

  if(argv[1] == NULL || 
    strcmp(argv[1], "~") == 0) {
    directory = getenv("HOME");
    if(directory == NULL) {
      fprintf(stderr, "Error: HOME not set\n");
      return 1;
    }
  } 
  else if(strcmp(argv[1], ".") == 0) {
    directory = ".";
  }
  else if(strcmp(argv[1], "..") == 0) {
    directory = "..";
  }
  else {
    directory = argv[1];
  }

  if(chdir(directory) == -1) {
    fprintf(stderr, "cd: could not change directory\n");
    return 1;
  }
  return 0;
} 

int built_in_cmd(Command *cmd) {
 
  BUILT_IN_CMD current_cmd = get_cmd(cmd->argv[0]);
  int status = 0;

  switch (current_cmd) {
    case CMD_CD:
      status = cmd_cd(cmd->argv);
      break;
    case CMD_PWD:
      // Linux's buffer is usualy 4096 long, while MacOS and BSDs use 1024
      char buffer[1024];
      if(getcwd(buffer, 1024) == NULL) {
        perror("Unable to get PATH");
        status = 1;
      }
      else {
        printf("%s\n", buffer);
        status = 0;
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
      status = 0;
      break;
    default:
      fprintf(stderr, "oyster: command not found: %s\n", cmd->argv[0]);
      status = 127;
      break;
  }
  return status;
}


// Functions for handling commands, I/O and special characters

void redirect_io(Command *cmd, size_t i) {
  Command *cmd_ = &cmd[i];

  if(cmd_->stream.file_in != NULL){
    int fd = open(cmd_->stream.file_in, O_RDONLY);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to read from %s\n", cmd_->stream.file_in);
      return;
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  else if(cmd_->stream.file_out != NULL && cmd_->stream.file_err != NULL) {
    int fd; 
    // Checks whether if appending has occured.
    int ff = cmd_->stream.stdio_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) :
                                       (O_WRONLY | O_CREAT | O_TRUNC);
    fd = open(cmd_->stream.file_out, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error unable to write output and error to %s\n", cmd_->stream.file_out);
      return;
    }
    fflush(stdout);
    dup2(fd, STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(fd);
  }
  else if(cmd_->stream.file_out != NULL){
    int fd; 
    int ff = cmd_->stream.stdio_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) :
                                       (O_WRONLY | O_CREAT | O_TRUNC);
    fd = open(cmd_->stream.file_out, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to write output to %s\n", cmd_->stream.file_out);
      return;
    }
    fflush(stdout);
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  else if(cmd_->stream.file_err != NULL) {
    int fd; 
    int ff = cmd_->stream.stderr_append != 0 ? (O_WRONLY | O_CREAT | O_APPEND) : 
                                        (O_WRONLY | O_CREAT | O_TRUNC);
            
    fd = open(cmd->stream.file_err, ff, 0644);
    if(fd < 0) {
      fprintf(stderr, "Error: unable to write error to %s\n", cmd_->stream.file_err);
      return;
    }
    fflush(stdout);
    dup2(fd, STDERR_FILENO);
    close(fd);
  }
}

void handle_special_characters(Pipeline* const pl, int last_status) {

  // Large enough for 32-bit numbers.
  char buffer[12];

  for(size_t i = 0; i < pl->cmds_count; i++) {

    Command *cmds = &pl->cmds[i];

    size_t j = 0;
    while(cmds->special_ch_count > 0 && j < cmds->argc) {
      if(strcmp(cmds->argv[j], "$?") == 0) {
        snprintf(buffer, sizeof(buffer), "%d", last_status);

        free(cmds->argv[j]);
        cmds->argv[j] = strdup(buffer);

        cmds->special_ch_count--;
      }
      j++;
    }
  }
  return;
}


// Handle Commands


void handle_single_cmd(Command* const cmd, int *status) {
  BUILT_IN_CMD command = get_cmd(cmd->argv[0]);

  // If not redirection in an external command has occurred.
  if(command == CMD_EXTERNAL && cmd->stream.file_in == NULL && cmd->stream.file_out == NULL && cmd->stream.file_err == NULL) {
    pid_t pid = fork();

    if(pid == -1) {
      fprintf(stderr, "Fatal: child process failed to execute command\n");
      *status = 1;
      return;
    }
    if(pid == 0) {
      execvp(cmd->argv[0], cmd->argv);
      fprintf(stderr, "Error: unable to execute command\n");
      exit(127);
    }

    waitpid(pid, status, 0);
  }
  // Otherwise if redirection in both external and/or built-in has occured.
  else if(cmd->stream.file_in != NULL || cmd->stream.file_out != NULL || cmd->stream.file_err != NULL) {
    pid_t pid = fork();
    if(pid == -1) {
      fprintf(stderr, "Fatal: child process failed to execute command\n");
      *status = 1;
      return;
    }
    if(pid == 0) {
      redirect_io(cmd, 0);
      if(command == CMD_EXTERNAL) {
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "Error: unable to execute command\n");
        exit(127);
      }
      else {
        int built_in_status = built_in_cmd(&cmd[0]);
        *status = built_in_status;
        exit(built_in_status);
      }
    }
    waitpid(pid, status, 0);
  }
  // Executed built-in commands only.
  else { 
    *status = built_in_cmd(&cmd[0]);
    // Converts it to a format waitpid() and exit() uses, this makes it so that
    // WEXITSTATUS() and WIFEXITED can understand.
    *status <<= 8;
  }
}

void handle_pipes(Command *cmd, size_t cmd_count, int *status) {
    
  size_t number_of_pipes = cmd_count - 1;

  int pipes[number_of_pipes][2];
  pid_t pids[cmd_count];

  for(size_t i = 0; i < number_of_pipes; i++) {
    if(pipe(pipes[i]) == -1) {
      fprintf(stderr, "Fatal: pipe failed to initialise\n");
      all_commands_free(cmd, cmd_count);
      *status = 1;
      return;
    }
  }

  for(size_t i = 0; i < cmd_count; i++) {
    pids[i] = fork();
    if(pids[i] == -1) {
      fprintf(stderr, "Fatal: failed to create process\n");
      all_commands_free(cmd, cmd_count);
      *status = 1;
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

      if(cmd[i].stream.file_in != NULL || 
        cmd[i].stream.file_out != NULL || 
        cmd[i].stream.file_err != NULL) { redirect_io(cmd, i); }

      if(execvp(cmd[i].argv[0], cmd[i].argv) == -1) {
        fprintf(stderr, "Fatal: child process failed to execute command\n");
        exit(127);
      }
    }
  }
  for(size_t j = 0; j < number_of_pipes; j++) {    
    close(pipes[j][0]); 
    close(pipes[j][1]); 
  }

  for(size_t j = 0; j < cmd_count; j++) { waitpid(pids[j], status, 0); }
}

ExecutionUnit *handle_parsed_units(Token *t, size_t *units_count) {

  // Declare the amount of units we are going to have.
  ExecutionUnit *units = NULL;

  // Amount of units that can be stored.
  size_t capacity = 0;
  // Number of current units.
  size_t count = 0;

  Pipeline pipeline;

  while(t != NULL) {
    Token *start = t;
    OperatorType operator = OP_NONE;

    // Advances t until it hits either a &&, || or the end of a string.
    while(t != NULL) {
      if(t->token_type == OPERATOR) {
        operator = get_operator(t->content);
        if(operator == OP_CONDITIONAL_OR ||
           operator == OP_CONDITIONAL_AND) {
          break;
        }
      }
      t = t->next;
    }

    // Checks whether if any commands exist in the pipeline.
    pipeline.cmds = parse_cmds(start, &pipeline.cmds_count);
    if(pipeline.cmds == NULL || pipeline.cmds_count == 0) {
      break;
    }


    // Assigns data to a new unit.
    ExecutionUnit unit;
    unit.pl = pipeline;
    unit.op_type = OP_NONE;

    // Checks whether if && or || occurred, 
    // if so it sets the operator to the detected operator and advances to the next unit.
    if(t != NULL && t->token_type == OPERATOR) {
      if(operator == OP_CONDITIONAL_AND || operator == OP_CONDITIONAL_OR) {
        unit.op_type = operator;
        // Skip && or ||
        t = t->next;
      }
    }

    // Checks whether if the count of units reaches or exceeds the total capacity.
    if(capacity == count) {
      capacity = capacity > 0 ? capacity * 2 : 4;
      ExecutionUnit *units_ = realloc(units, capacity * sizeof(*units));
      if(units_ == NULL) {
        fprintf(stderr, "Fatal: unable to allocate memory to execution unit\n");
        exit(EXIT_FAILURE);
      }
      units = units_;
    }

    // Stores the current unit to the units dynamic array.
    units[count++] = unit;
  } 

  // Gets the total number of units.
  *units_count = count;
  return units;
} 

int handle_exec_status(Pipeline* const pl) {

  int status = 0;

  if(strcmp(pl->cmds->argv[0], "exit") == 0) {
    fprintf(stdout, "oyster: Exiting shell\n");
    exit(EXIT_SUCCESS);
  }

  if(pl->cmds_count > 1) {
    handle_pipes(pl->cmds, pl->cmds_count, &status);
    if(WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return 1;
  }
  else if(pl->cmds_count == 1){
    handle_single_cmd(&pl->cmds[0], &status);
    if(WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return 1;
  }
  return 0;
}

void handle_exec_units(ExecutionUnit* unit, size_t units_count) {
  // Default value
  int last_status = 0;

  for(size_t i = 0; i < units_count; i++) {
    // It enters this if statement only if 2 execute units exist. 
    if(i > 0) {
      OperatorType previous_unit = unit[i - 1].op_type;
      if(previous_unit == OP_CONDITIONAL_AND && last_status != 0) {
        continue;
      }
      if(previous_unit == OP_CONDITIONAL_OR && last_status == 0) { 
        continue;
      }
    }
      
    handle_special_characters(&unit[i].pl, g_last_status);
    last_status = handle_exec_status(&unit[i].pl);
    g_last_status = last_status;
  }
}
