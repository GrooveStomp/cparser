All source code here uses the Createive Commons Attribution 4.0 International License.
See: https://creativecommons.org/licenses/by/4.0/
See: https://creativecommons.org/licenses/by/4.0/legalcode
Attribution is appreciated, though not required.

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
- Associate identifier names with occurence of that token in the stream
- operators: + - *
- real numbers 4e23, 3.266, 0.984F

## New Algorithm proposal:

    buffer = open_file('main.c')
    head = buffer

    do
      var curr <- buffer[head]
      terminal_clause_function_ptr <- determine_terminal_clause(curr)
      /*
        We don't know what the token is yet, so we need to peek ahead or guess
        based on the current character.
        If we encounter a '/', then we may have a division symbol, or maybe
        the start of a comment.  We can look ahead and see if the next character
        is a '*' and if so, then we have a comment. Otherwise assume division.
        OR:
        We can say the termination clause is to read '*' immediately, then later
        on to read "*/" together; or to simply read any character other than '*'
        immediately next.  Whichever comes first.
      */


      sub_buffer <- generate_sub_buffer(buffer, head, terminal_clause_function_ptr)
      /* Do not allocate new memory for sub_buffer. */


      type = determine_token_type(sub_buffer)
      /*
        This is where the core of the lexer lives.  We can make note of which
        terminal clause was used and use that to limit the possibilities here.
        We can even have the terminal clause function keep note of which token
        type we have, provided the terminal clause function had enough
        information to figure that out. eg.: '/' vs. '/*' for divide or comment.
      */


      print(type, sub_buffer)

      head += sizeof(sub_buffer)

    while (curr != EOF)