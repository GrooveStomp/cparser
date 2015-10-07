Lexer and tokenizer for the C language.
Reads itself (main.c) and identifies various tokens therein.

# Dependencies
- C compiler (Assuming GCC for the remainder of this README)

# Building
```
gcc -g -x c main.c
# -g for gdb support
# -x to specify the language
```

# Running
```
./a.out local_file
```

# Debugging
Build with `-g` option.  Eg.,: `gcc -g main.c`

```
gdb --args a.out local_file
```

# TODO:
- Support 'L' prefix for string literals
- Doesn't read '"' properly for string literals
  - <TokenStringLiteral> "') && !CurrentState.StringLiteral && !CurrentState.LineComment && !BlockComment) {
        CurrentState.StringLiteral = true;
        continue;
      }
      else if ((InputChar == '"

- Associate identifier names with occurence of that token in the stream
- operators: + - *
- character constants: '\0'
- real numbers 4e23, 3.266, 0.984F