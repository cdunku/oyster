# Oyster Shell

A simple Unix-like shell implemented in C. 
It has served me as a learning tool on creating a minimal, but functional command interpeter with its own tokenizer, parser, redirection, process control and shell internals.


## Current features

- Implemented my own `getline()` function
- Implemented my own `tokenizer` and `command parser`
- Support for multiple commands (`|`).
- Built-in commands: `cd`, `exit`, `pwd`, `echo`

## Upcoming features

### Redirectors and Logical Operators
- Adding support for the following redirectors: `<`, `>`, `<<`, `>>`, `&>`, `2>`
- Adding support for the following logical operators: `&&`, `||`
  
### Built-in Commands with and without redirectors
- This is currently semi-functional with built-in commands being always executed in a child process. I am working on a fix.

### History + Readline
- Interactive line editing
- Command line history

### Job Control
- Background execution (`&`)
- `jobs` list
- `fg`/`bg`
- Proper process grouping and signal handling

### Globbing
- The use of wildcard expansions (`*`, `?`)

### Environment Variables (Maybe)
- Variable expansion (`$HOME`, `$PATH`)

### Parser improvements
- Currently there are some problems when running `|` inside a `" "` or `' '`, I will start fixing that soon.
- Improve readability, maintainability and correctness.


## How to build and run?

### Building release
Just running `make` compiles with flags for `production` and `testing`, for the `release` run:
```bash
make release
```
Run the executable:
```bash
./oyster
```
You will be inside the shell when you see:
```bash
$ -> 
```
