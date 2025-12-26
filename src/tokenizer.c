#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"
#include "input.h"
#include "helper.h"
#include "exec.h"


#define CAPACITY 64

/* Helper functions */

// Checks whether if an escape character exists.
bool esc_ch_exists(const char ch) {
  switch (ch) {
    case '\n':
    case '\t':
    case ' ': return true;
    default: return false;
  }
  return false;
}

// Checks for both escape and special characters.
bool check_for_esc_and_spec_ch(const char *str, size_t i, size_t *consumed, char *operator) {
  const char ch = str[i];
  
  switch(ch) {
    case '\n':
    case '\t':
    case ' ': {
      operator[0] = '\0';
      *consumed = 0;
      return true;
    }
    case '$': {
      if(str[i + 1] == '?') {
        operator[0] = ch;
        operator[1] = '?';
        operator[2] = '\0';
        *consumed = 1;
        return true;
      }
    }
    default: { 
      operator[0] = '\0';
      return false;
    }
  }
  // If something goes wrong
  return false;
}

bool check_for_quotes(const char ch, bool *double_quotes, char *quote) {
  switch (ch) {
    case '"': *double_quotes = true;
    case '\'': *quote = ch; return true;
    
    default: return false;
  }
  // If something goes wrong
  return false;
}
bool check_for_redirector_operator(const char *str, size_t i, size_t *consumed, char *operator) {
  char ch = str[i];
  switch (ch) { 
    case '|':
    case '<': {
      if(str[i + 1] != ch) {
        operator[0] = ch;
        operator[1] = '\0';
        *consumed = 0;
        return true;
      }
    }
    case '>': {
      if(str[i + 1] == ch) {
        operator[0] = ch;
        operator[1] = '>';
        operator[2] = '\0';
        *consumed = 1;
        return true;
      }
      else {
        operator[0] = ch;
        operator[1] = '\0';
        *consumed = 0;
        return true;
      }
    }
    case '&':
    case '2': {
      if(str[i + 1] == '>' && str[i + 2] == '>') {
        operator[0] = ch;
        operator[1] = '>';
        operator[2] = '>';
        operator[3] = '\0';
        *consumed = 2;
        return true;
      }
      else if(str[i + 1] == '>') {
        operator[0] = ch;
        operator[1] = '>';
        operator[2] = '\0';
        *consumed = 1;
        return true;
      }
      else {
        operator[0] = '\0';
        return false;
      }
    }

    default: return false;
  }

  return false;
}

bool check_for_conditional_operator(const char *str, size_t i, size_t *consumed, char *operator) {
  char ch = str[i];
  switch (ch) {
    case '&': 
    case '|': {
      if(str[i + 1] == ch) {
        operator[0] = ch;
        operator[1] = ch;
        operator[2] = '\0';
        *consumed = 1;
        return true;
      }
    }
    default: {
      operator[0];
      return false;
    }
  }
  return false;
}
// For simplicity sake, we will only retrieve 2 hex digits and 3 octal digits.
char parse_hex_escape(const char *str, size_t i, size_t *hex_digits) {
  int val = 0;
  int count = 0;

  // Read up to two hex digits
  for (int k = 0; k < 2; k++) {
    char c = str[i + k];
    int digit;

    if (c >= '0' && c <= '9') { digit = c - '0'; }
    else if (c >= 'a' && c <= 'f') { digit = c - 'a' + 10; }
    else if (c >= 'A' && c <= 'F') { digit = c - 'A' + 10; } 
    else { break; }

    val = (val * 16) + digit;
    count++;
  }

  *hex_digits = count;
  return (char)val;
}

char parse_octal_escape(const char *str, size_t i, size_t *oct_digits) {
  int val = 0;
  int count = 0;

  // Read up to three octal digits
  while (count < 3) {
    char c = str[i + count];
    if (c < '0' || c > '7') { break; }

    val = (val * 8) + (c - '0');
    count++;
  }

  *oct_digits = count;
  return (char)val;
}

// We check whether if there is a possibility for executing special characters.
bool handle_double_quotes(const char *str, size_t *consumed, char *esc_ch) {
  char c = str[0];

  switch (c) {
    case 'a': *esc_ch = '\a'; *consumed = 1; return true;
    case 'b': *esc_ch = '\b'; *consumed = 1; return true;
    case 'f': *esc_ch = '\f'; *consumed = 1; return true;
    case 'n': *esc_ch = '\n'; *consumed = 1; return true;
    case 'r': *esc_ch = '\r'; *consumed = 1; return true;
    case 't': *esc_ch = '\t'; *consumed = 1; return true;
    case 'v': *esc_ch = '\v'; *consumed = 1; return true;
    case '?': *esc_ch = '\?'; *consumed = 1; return true;
    case '\\': *esc_ch = '\\'; *consumed = 1; return true;
    case '"': *esc_ch = '"'; *consumed = 1; return true;
    case '\'': *esc_ch = '\''; *consumed = 1; return true;

    case 'x': {
      size_t hex_count = 0;
      *esc_ch = parse_hex_escape(str + 1, 0, &hex_count);
      if (hex_count == 0) {
        *esc_ch = 'x';
        *consumed = 1;
      } else {
        *consumed = 1 + hex_count;
      }
      return true;
    }

    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': {
      size_t oct_count = 0;
      *esc_ch = parse_octal_escape(str, 0, &oct_count);
      *consumed = oct_count;
      return true;
    }

    default: {
      *esc_ch = c;
      *consumed = 1;
      return true;
    }
  }
}


// We create this local struct with the purpose of keeping track of the current token being tokenized.
typedef struct TokBuf {

  char *buffer;
  size_t length;
  size_t capacity;

} TokBuf;


// With this function we add the token from the *buffer from TokBuf to the Token struct's *content.
// 
// If we just used *content and check for boundaries, 
// we would need to keep track of the size and realloc() every time.
//
// Additionally, finding the length of the token through end - start, 
// where end = str + i and start = 0 in the beginning 
// (while we would need to update start to be start = 0 after the first token).
// would make the whole length finding unnecessarily complex.
void token_add(TokBuf *tb, Token **head, Token **tail, TokenType type) {
  
  if(tb->length == 0) { return; }

  Token *token = malloc(sizeof(Token));
  if(token == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to token\n");
    exit(EXIT_FAILURE);
  }

  token->content = malloc((tb->length + 1) * sizeof(char));
  if(token->content == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to content\n");
    exit(EXIT_FAILURE);
  }

  // Copies everything from the tb->buffer to the actual token.
  memcpy(token->content, tb->buffer, tb->length);
  token->content[tb->length] = '\0';
  token->token_type = type;
  token->next = NULL;

  // *tail helps us track and append to the *head list.
  // Don't forget that both are pointers and edit memory locations!
  //
  // When tail points to the same node like head,
  // and we edit tail to point the the next token, the head also gets updated.
  if(*tail != NULL) {
    (*tail)->next = token;
  }
  else {
    *head = token;
  }
  *tail = token;

  tb->length = 0;
}

// Helps us add characters step-by-step into the temporary "token" *buffer.
void append_ch(TokBuf *tb, char ch) {
  if(tb->capacity == 0) {
    tb->capacity = CAPACITY;
    tb->buffer = malloc(tb->capacity * sizeof(char));
  } 
  else if(tb->capacity <= tb->length + 1) {
    tb->capacity *= 2;
    tb->buffer = realloc(tb->buffer, tb->capacity * sizeof(char));
  }

  // Sets the next index inside the buffer to '\0' just in case we encounter the end,
  // because we do not know the exact size of the array.
  //
  // When we need to add a new character, we just overwrite the existing '\0' and add a new one.
  tb->buffer[tb->length++] = ch;
  tb->buffer[tb->length] = '\0';
}
void append_op(Token **head, Token **tail, char *redirect_operator, TokenType type) {

  Token *operator = malloc(sizeof(Token));
  operator->token_type = type;
  operator->content = strdup(redirect_operator);
  operator->next = NULL;
  if(tail != NULL) { 
    (*tail)->next = operator; 
  }
  else {
    *head = operator;
  }
  *tail = operator;
}

Token *tokenizer(const char *str) {

  TokBuf tb = { 0 };

  Token *head = NULL;
  Token *tail = NULL;

  size_t length = strlen(str);
  size_t i = 0;
  // The offset when tokenizing a redirector, escape character or special symbol.
  size_t consumed = 0;

  // Helps us keep track of quotes. 
  bool quotes = false, double_quotes = false;
  char save_quote = 0;

  char operator[4];

  while(length > i) {
    char c = str[i];

    if(check_for_esc_and_spec_ch(str, i, &consumed, operator) && !quotes) {
      if(tb.length > 0) {
        token_add(&tb, &head, &tail, STRING);
        continue;
      }
      if(consumed == 0 && esc_ch_exists(c)) { 
        i++;
      }
      else {
        append_op(&head, &tail, operator, SPECIAL);
        i += 1 + consumed;
      }
      continue;
    }
   
    // Checks whether if we have encountered a string.
    if(check_for_quotes(c, &double_quotes, &save_quote) && !quotes) {
      quotes = true;
      i++;
      continue;
    }
    // If we encounter a double quote ("), an escape sequence (\) and are inside quotes,
    // check whether if we should handle special cases (escape characters).
    if(quotes && double_quotes && str[i] == '\\') {
      char esc_ch;
      size_t esc_ch_size = 0;
      if(handle_double_quotes(str + i + 1, &esc_ch_size, &esc_ch)) {
        append_ch(&tb, esc_ch);
        i += 1 + esc_ch_size;
        continue;
      }
    }
    // If we have reached the same quote sign, exit the string.
    if((str[i] == save_quote) && quotes) {
      quotes = false;
      i++;
      continue;
    }

    if(check_for_conditional_operator(str, i, &consumed, operator) && !quotes) {
      // Adds the command being added into the buffer into a node inside the list.
      token_add(&tb, &head, &tail, STRING);
      // Append the operator to the list using a node.
      append_op(&head, &tail, operator, OPERATOR);
      i += 1 + consumed;
      continue;
    }
    if(check_for_redirector_operator(str, i, &consumed, operator) && !quotes) {
      // Same logic appears here.
      token_add(&tb, &head, &tail, STRING);
      append_op(&head, &tail, operator, OPERATOR);
      i += 1 + consumed;
      continue;
    }


    // Increment to the next index, and stream.stdio_append the character to *buffer.
    i++;
    append_ch(&tb, c);
  }

  // Finally, add the last token to our linked list and return the list.
  token_add(&tb, &head, &tail, STRING);
  free(tb.buffer);

  return head;
}


bool handle_stream(Command *cmd, OperatorType operator, const char *filename) {
  RedirectionStream *s = &cmd->stream;

  switch (operator) {
    case OP_REDIRECT_IN: {
      if(s->file_in != NULL) { free(s->file_in); }

      s->file_in = strdup(filename);
      return true;
    }
    case OP_REDIRECT_OUT:
    case OP_REDIRECT_OUT_APPEND: {
      if(s->file_out != NULL) { free(s->file_out); }

      s->file_out = strdup(filename);
      s->stdio_append = (operator == OP_REDIRECT_OUT_APPEND);
      return true;
    }
    case OP_REDIRECT_ERR:
    case OP_REDIRECT_ERR_APPEND: {
      if(s->file_err != NULL) { free(s->file_err); }

      s->file_err = strdup(filename);
      s->stderr_append = (operator == OP_REDIRECT_ERR_APPEND);
      return true;
    }
    case OP_REDIRECT_ALL_APPEND:
    case OP_REDIRECT_ALL: {
      if(s->file_out != NULL) { free(s->file_out); }
      if(s->file_err != NULL) { free(s->file_err); }

      s->file_out = strdup(filename);
      s->file_err = strdup(filename);

      s->stdio_append = (operator == OP_REDIRECT_ALL_APPEND);
      s->stderr_append = false;
      return true;
    }
  }
  return false;
}
// Does a check whether if a redirector occured using the range of OperatorType enum.
bool is_redirector(OperatorType operator) {
  return operator >= OP_REDIRECT_IN && operator <= OP_REDIRECT_ALL_APPEND;
}

Command *handle_operator(Command *cmd, Token **t, size_t *i, size_t *cmd_count, size_t *current_cmd, OperatorType operator) {

  if(is_redirector(operator)) {
    // Skip the stdin/stdout/stderr stream.
    (*t) = (*t)->next;
    if(*t == NULL) {
      fprintf(stderr, "Error: parser error using redirection\n");
      return NULL;;
    }
    if(handle_stream(&cmd[*current_cmd], operator, (*t)->content)) { return cmd; }
  }
  else if(operator == OP_PIPE) {
    cmd[*current_cmd].argv[*i] = NULL;
    cmd[*current_cmd].argc = *i;
    
    *i = 0;
    (*cmd_count)++;
    (*current_cmd)++;

    Command *cmd_ = realloc(cmd, *cmd_count * sizeof(Command));
    if(cmd_ == NULL) {
      fprintf(stderr, "Fatal: unable to reallocate memory to parsed command\n");
      exit(EXIT_FAILURE);
    }
    cmd = cmd_;

    memset(&cmd[*current_cmd], 0, sizeof(Command));

    cmd[*current_cmd].argv = malloc(CAPACITY * sizeof(char *));
    if(cmd[*current_cmd].argv == NULL) {
      fprintf(stderr, "Fatal: unable to allocate memory to command vector\n");
      exit(EXIT_FAILURE);
    }

    return cmd;
  }
  return cmd;
}



// We parse the commands, manage the I/O stream for each command and handle redirectors.
Command *parse_cmds(Token *t, size_t *total_cmd) {

  // Store the head of the list for freeing memory
  Token *head = t;

  size_t cmd_count = 1, current_cmd = 0;
  Command *cmd = malloc(cmd_count * sizeof(Command));
  if(cmd == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to parse commands\n");
    exit(EXIT_FAILURE);
  }

  size_t argv_capacity = CAPACITY;

  memset(&cmd[current_cmd], 0, sizeof(Command));
  cmd[current_cmd].argv = malloc(argv_capacity * sizeof(char *));
  if(cmd[current_cmd].argv == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to command vector\n");
    exit(EXIT_FAILURE);
  }

  // Go throught the list and copy it to the arguments vector.
  size_t i = 0;
  while(t != NULL) {
    if(t->token_type == OPERATOR) {

      argv_capacity = CAPACITY;

      OperatorType operator = get_operator(t->content);

      // If a && or || occurs, then end the pipeline and command parsing.
      if(operator == OP_CONDITIONAL_AND || operator == OP_CONDITIONAL_OR) { break; }

      Command *_cmd = handle_operator(cmd, &t, &i, &cmd_count, &current_cmd, operator);
      if(_cmd == NULL) {
        fprintf(stderr, "Error: unable to retrieve redirect operator\n");
        break;
      }
      cmd = _cmd;
     
      // Skip the operator or file stream.
      t = t->next;
      continue;
    }
    if(t->token_type == SPECIAL) {
      cmd[current_cmd].special_ch_count++;
    } 
    cmd[current_cmd].argv[i++] = strdup(t->content);

    if(i + 1 >= argv_capacity) {
      argv_capacity *= 2;
      char **argv_ = realloc(cmd[current_cmd].argv, argv_capacity * sizeof(char *));
      if(argv_ == NULL) {
        fprintf(stderr, "Error: unable to reallocate memory to command\n");
        vector_free(cmd[current_cmd].argv, i);
        token_list_free(head);
        exit(EXIT_FAILURE);
      }
      cmd[current_cmd].argv = argv_;
    }
    t = t->next;
  }

  if(i > 0) {
    cmd[current_cmd].argv[i] = NULL;
    cmd[current_cmd].argc = i;
  }

  *total_cmd = cmd_count;

  return cmd;
}
