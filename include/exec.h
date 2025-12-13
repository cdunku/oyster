#pragma once

#include "tokenizer.h"

typedef enum {
  // Built-in commands 
  CMD_CD,
  CMD_ECHO,
  CMD_PWD,
  CMD_HISTORY,
  CMD_TYPE,
  CMD_ALIAS,
  CMD_UNALIAS,
  CMD_EXPORT,
  CMD_HELP,
  
  CMD_EXTERNAL,

  CMD_UNKNOWN,
} BUILT_IN_CMD;

void handle_exec(Command *cmd, size_t cmd_count);
