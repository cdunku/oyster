#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#include "glob.h"

#include "tokenizer.h"

// Helper functios

char *substring(const char *src, size_t start, size_t len) {
  char *dest = malloc(len + 1);
  if(dest == NULL) {
    fprintf(stderr, "Error: unable to allocate memory to substring\n");
    return NULL;
  }

  memcpy(dest, src + start, len);
  dest[len] = '\0';

  return dest;
}

bool glob_match(const char* pattern, const char* entry) {
  const char *p = pattern;
  const char *e = entry;

  const char *asterisk = NULL;
  // After reaching *, it goes through each filename looking through patterns.
  const char *asterisk_match = NULL;

  while(*e != '\0') {
    if(*p == *e) {
      p++;
      e++;
    }
    else if(*p == '*') {
      asterisk = p;
      asterisk_match = e;
      p++; // Move past *
    }
    else if(asterisk != NULL) {
      p = asterisk + 1;
      asterisk_match++;
      e = asterisk_match;
    }
    else {
      return false;
    }
  }

  while(*p == '*') {
    p++;
  }

  return *p == '\0';
}

char **check_filesystem_for_glob(const char* glob_str, size_t *count) {

  // The initial amount of matches we can accept.
  size_t capacity = 4;
  char **matches = malloc(capacity * sizeof(char *));
  if(matches == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for matches in filesystem.\n");
    exit(EXIT_FAILURE);
  }


  *count = 0;

  // Parse the pattern and directory of the glob
  const char *slash = strrchr(glob_str, '/');

  char *directory = NULL;
  char *pattern = NULL;

  if(slash == NULL) {
    // 1 byte for "." and 1 byte for '\0'.
    directory = strdup(".");
    pattern = strdup(glob_str);
  }
  else if(slash == glob_str) {
    directory = strdup("/");
    pattern = strdup(slash + 1);
  }
  else {
    size_t dir_len = slash - glob_str;
    directory = substring(glob_str, 0, dir_len);
    pattern = strdup(slash + 1);
  }


  // Open/Read/Close the retrieved directory.
  
  DIR *dir; // Helps us access the directory.
  struct dirent *entry; // Stores the informations of directories and files.

  dir = opendir(directory);
  if(dir == NULL) {
    fprintf(stderr, "Error: unable to access %s\n", directory);
    exit(EXIT_FAILURE);
  }

  while((entry = readdir(dir)) != NULL) {
    if(!glob_match(pattern, entry->d_name)) { continue; }
    if(*count == capacity) {
      capacity *= 2;
      char **matches_ = realloc(matches, capacity * sizeof(char *));
      if(matches_ == NULL) {
        fprintf(stderr, "Error: unable to allocate new matches\n");
        free(matches);
        exit(EXIT_FAILURE);
      }
      matches = matches_;
    }
    if(strcmp(directory, ".") == 0) {
      matches[*count] = strdup(entry->d_name);
    }
    else {
      size_t path_len = strlen(directory) + strlen(entry->d_name) + 2;
      char *path = malloc(path_len);
      snprintf(path, path_len, "%s/%s", directory, entry->d_name);
      matches[*count] = path;
    }
    (*count)++;
  }

  if(closedir(dir) == -1) {
    fprintf(stderr, "Error: unable to close %s\n", directory);
    exit(EXIT_FAILURE);
  }
  free(directory);
  free(pattern);

  return matches;
}

void expand_glob_for_command(Pipeline *pl) {

  Command *cmd = pl->cmds;

  size_t capacity = cmd->argc * 4 + 1;
  char **argv_ = malloc(capacity * sizeof(char *));
  if(argv_ == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for glob vector\n");
    exit(EXIT_FAILURE);
  }
  OperatorType *str_type_ = malloc(capacity * sizeof(OperatorType));
  if(str_type_ == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory for vector type\n");
    exit(EXIT_FAILURE);
  }

  size_t k = 0;

  for(size_t i = 0; i < cmd->argc; i++) {
    if(cmd->str_type[i] == GLOB) {

      char **matches = NULL;
      size_t matches_count = 0;

      matches = check_filesystem_for_glob(cmd->argv[i], &matches_count);
    
      // Checks whether if an overflow has occurred before adding any new elements.
      if(k + 1 >= capacity) {

        capacity *= 2;
        char **argv__ = realloc(argv_, capacity * sizeof(char *));
        OperatorType *str_type__ = realloc(str_type_, capacity * sizeof(OperatorType));

        if(argv__ == NULL || str_type__ == NULL) {
          fprintf(stderr, "Fatal: failed to reallocate memory to new argument vector\n");
          exit(EXIT_FAILURE);
        }

        argv_ = argv__;
        str_type_ = str_type__;
      } 

      if(matches != NULL && matches_count > 0) {
        for(size_t j = 0; j < matches_count; j++) {
          // Checks every time whether if an overflow has occured before adding matches.
          if(k + 1 >= capacity) {
            capacity *= 2;
            char **argv__ = realloc(argv_, capacity * sizeof(char *));
            OperatorType *str_type__ = realloc(str_type_, capacity * sizeof(OperatorType));

            if(argv__ == NULL || str_type__ == NULL) {
            fprintf(stderr, "Fatal: failed to reallocate memory to new argument vector\n");
            exit(EXIT_FAILURE);
            }

            argv_ = argv__;
            str_type_ = str_type__;
          } 
          // Append matches to argv_ as a STRING
          argv_[k] = matches[j];
          str_type_[k] = STRING;
          k++;
        }
        free(matches);
      }
      else {
        // Append everything normally as a STRING
        argv_[k] = strdup(cmd->argv[i]);
        str_type_[k] = STRING;
        free(matches);
        k++;
      }
    } else {
      // Appened cmd->argv[i] to argv_ as a STRING
      argv_[k] = strdup(cmd->argv[i]); 
      str_type_[k] = cmd->str_type[i];
      k++;
    }
  }

  argv_[k] = NULL;

  for (size_t i = 0; i < cmd->argc; i++) {
    free(cmd->argv[i]);
  }
  free(cmd->argv);
  if(cmd->str_type != NULL) { free(cmd->str_type); }

  cmd->argv = argv_;
  cmd->argc = k;
  cmd->str_type = str_type_;
}
