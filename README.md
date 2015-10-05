Lexer and tokenizer for the C language.
Reads itself (main.c) and identifies various tokens therein.

# Dependencies
- C compiler (Assuming GCC for the remainder of this README)

# Building
```
gcc main.c # OR:
gcc -g -x c main.c # debug symbols and specify "C" language
```

# Running
```
./a.out
```

# TODO:
- Associate identifier names with occurence of that token in the stream
- operators: + - *
- string literals: "As an example"
- character constants: '\0'
- integers (including hex, octal, binary prefix; L suffix?) 0xdeadbeef
- real numbers 4e23, 3.266, 0.984F