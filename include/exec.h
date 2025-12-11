#pragma once

#include "tokenizer.h"

typedef enum {
  // Built-in commands 
  CMD_EXIT,
  CMD_CD,
  CMD_ECHO,
  CMD_PWD,
  CMD_HISTORY,
  CMD_TYPE,
  CMD_ALIAS,
  CMD_UNALIAS,
  CMD_EXPORT,
  CMD_HELP,
  
  CMD_PIPE_REDIRECT,
  CMD_OUTPUT_REDIRECT,
  CMD_APPEND_REDIRECT,
  CMD_INPUT_REDIRECT,
  CMD_ERROR_REDIRECT,
  CMD_BOTH_REDIRECT, // Redirects both the output and error stream.
  // External commands inputted
  CMD_IS_COMMAND,
  CMD_EXTERNAL,

  CMD_UNKNOWN,
} BUILT_IN_CMD;

void handle_exec(Command *cmd, size_t cmd_count);
