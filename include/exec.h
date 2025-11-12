#pragma once

typedef enum {
  // Built-in commands 
  CMD_EXIT,
  CMD_CD,
  
  // External commands
  CMD_LS,
  CMD_GREP,

  // Unknown command inputted 
  CMD_UNKNOWN,
} CMD_TOOLS;

void handle_exec(char **argv, char **envp);
