#pragma once

typedef enum {
  // Built-in commands 
  CMD_EXIT,
  CMD_CD,
  
  // External commands inputted 
  CMD_EXTERNAL,

  CMD_UNKNOWN,
} BUILT_IN_CMD;

void handle_exec(char **argv, char **envp);
