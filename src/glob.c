#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"


char **check_filesystem_for_glob(const char* glob_str, size_t *count) {

  // The initial amount of matches we can accept.
  size_t capacity = 4;
  char **matches = malloc(capacity * sizeof(char *));
  if(matches == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for matches in filesystem.\n");
    exit(EXIT_FAILURE);
  }

  size_t str_len = strlen(glob_str);
  char directory[str_len];

  size_t last_backslash_pos = 0;
  size_t j = 0;

  for(size_t i = 0; i < str_len; i++) {

    if(glob_str[i] == '/') {
      last_backslash_pos = i;
    }
    directory[j++] = glob_str[i];
  }

  directory[last_backslash_pos] = '\0';

  size_t pattern_len = (str_len - last_backslash_pos) + 1;
  char pattern[pattern_len];

  // Copy the pattern from the last '/' into pattern[].
  for(size_t i = 0; i < pattern_len; i++) {
    pattern[i] = glob_str[last_backslash_pos + i]; 
  }
  pattern[pattern_len] = '\0';

  for(size_t i = 0; i < capacity; i++) {
    matches = 
  }
}

Command *expand_glob_for_command(Command *cmd) {
  size_t argc_ = cmd->argc * 2;
  char **argv_ = malloc(argc_ * sizeof(char *));
  if(argv_ == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for glob vector\n");
    exit(EXIT_FAILURE);
  }
  OperatorType *str_type_ = malloc(argc_ * sizeof(OperatorType));
  if(str_type_ == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for vector type\n");
    exit(EXIT_FAILURE);
  }

  for(size_t i = 0; i < cmd->argc; i++) {
    if(cmd->str_type[i] == GLOB) {

      char **matches = NULL;
      size_t matches_count = 0;

      matches = check_filesystem_for_glob(cmd->argv[i], &matches_count);
      
      if(matches != NULL) {
        for(size_t j = 0; j < matches_count; j++) {
          // Append matches to argv_ as a STRING
        }
      }
      else {
        // Append everything normally as a STRING
      }
    } else {
      // Appened cmd->argv[i] to argv_ as a STRING
    }
  }

  cmd->argv = argv_;
  cmd->argc = argc_;
  cmd->str_type = str_type_;

  return cmd;
}

bool match_filename_pattern(const char *filename, const char* pattern) {

}

void split_glob_pattern(const char* pattern);


