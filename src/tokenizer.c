#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"
#include "helper.h"

#define CAPACITY 64

/* Helper functions */

bool check_for_esc_ch(const char ch) {
  switch(ch) {
    case '\n':
    case '\t':
    case ' ': return true;
    default: return false;
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
  strncpy(token->content, tb->buffer, tb->length + 1);
  token->content[tb->length] = '\0';
  token->type = type;
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

Token *tokenizer(const char *str) {

  TokBuf tb = { 0 };

  Token *head = NULL;
  Token *tail = NULL;

  size_t length = strlen(str);
  size_t i = 0;

  // Helps us keep track of quotes. 
  bool quotes = false, double_quotes = false;
  char save_quote = 0;

  while(length > i) {
    char c = str[i];

    if(check_for_esc_ch(c) && !quotes) {
      token_add(&tb, &head, &tail, STRING);
      i++;
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

    // Increment to the next index, and append the character to *buffer.
    i++;
    append_ch(&tb, c);
  }

  // Finally, add the last token to our linked list and return the list.
  token_add(&tb, &head, &tail, STRING);
  free(tb.buffer);

  return head;
}

size_t tokenized_list_size(Token *head) {
  size_t size = 0;

  while(head != NULL) {
    size++;
    head = head->next;
  }

  return size;
}

// We parse the commands, manage the I/O stream for each command and handle redirectors.
Command *parse_cmds(Token *t) {
  Command *cmd = malloc(sizeof(Command));
  if(cmd == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to Command struct\n");
    exit(EXIT_FAILURE);
  }

  size_t length = tokenized_list_size(t);

  cmd->argv = malloc((length + 1) * sizeof(char *));
  if(cmd->argv == NULL) {
    fprintf(stderr, "Fatal: unable to allocate memory to command vector\n");
    exit(EXIT_FAILURE);
  }

  // Go throught the list and copy it to the arguments vector.
  size_t i = 0;
  while(t != NULL) {
    cmd->argv[i++] = strdup(t->content);
    t = t->next;
  }

  cmd->argv[i] = NULL;
  cmd->argc = length;

  return cmd;
}
