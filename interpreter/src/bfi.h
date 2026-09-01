#ifndef BFI_H_
#define BFI_H_

#include "basic.h"

typedef enum {
     TOK_RIGHT      = '>',
     TOK_LEFT       = '<',
     TOK_ADD        = '+',
     TOK_SUB        = '-',
     TOK_PUT        = '.',
     TOK_GET        = ',',
     TOK_JZ         = '[',
     TOK_JNZ        = ']',
     TOK_ZERO,      // [-] or [+]
} Token_Kind;

typedef struct {
     Token_Kind kind;
     size_t operand;
} Token;

typedef struct {
     Token *items;
     size_t count;
     size_t capacity;
} Tokens;

typedef struct {
     size_t *items;
     size_t count;
     size_t capacity;
} Bracket_Pos;

#define bp_push stack_push
#define bp_pop stack_pop
#define bp_peek_last stack_peek_last
#define bp_empty stack_empty

typedef struct {
     const char *filepath;
     size_t line;
     size_t col;
     const char *reason;
} Error;

typedef struct {
     Error *items;
     size_t count;
     size_t capacity;
} Errors;

#define errs_push stack_push
#define errs_empty stack_empty

void errors_print(const Errors *errors);

typedef struct {
     const char *filepath;
     String_Builder *source;
     size_t pos;
     size_t line;
     size_t col;
     Tokens tokens;
     Errors errors;
} Lexer;

void lexer_push(Lexer *lexer, Token token);
void lexer_tokenize(Lexer *lexer);
bool lexer_next(Lexer *lexer, Token *token);
bool lexer_peek(Lexer lexer, Token *token);
bool lexer_peekn(Lexer lexer, size_t n, Token *token);
bool lexer_peekn_loc(Lexer lexer, size_t n, Token *token, size_t *pos, size_t *line, size_t *col);
void lexer_skip_non_bf(Lexer *lexer);

void interpret(Tokens *tokens, unsigned char *memory);

#endif // BFI_H_
