#include "bfjit.h"

#define is_bf_cmd(c) (strchr("><+-.,[]", (c)) != NULL)
#define is_bf_cmd_mergeable(c) (strchr("><+-", (c)) != NULL)

void lexer_push(Lexer *lexer, Token token)
{
     da_append(&lexer->tokens, token);
}

void lexer_tokenize(Lexer *lexer)
{
     Bracket_Pos bp = {0};
     Token token;

     for (size_t i = 0; lexer_next(lexer, &token); i++) {
          if (token.kind == TOK_JZ) {
               bp_push(&bp, i);
          } else if (token.kind == TOK_JNZ) {
               if (!bp_empty(&bp)) {
                    token.operand = bp_peek_last(&bp);
                    lexer->tokens.items[bp_pop(&bp)].operand = i;
               } else {
                    errs_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unexpected closing bracket ']'"}));
               }
          }
          lexer_push(lexer, token);
     }

     for (size_t i = 0; i < bp.count; i++) {
          errs_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unclosed bracket '['"}));
     }
}

bool lexer_next(Lexer *lexer, Token *token)
{
     if (lexer->line == 0) lexer->line++;
     if (lexer->col == 0)  lexer->col++;

     lexer_skip_non_bf(lexer);

     if (lexer->pos >= lexer->source->count) return false;

     token->kind = lexer->source->items[lexer->pos];
     token->operand = 1;

     lexer->pos++;
     lexer->col++;

     if (is_bf_cmd_mergeable(token->kind)) {
          while (lexer->pos < lexer->source->count) {
               if (!is_bf_cmd(lexer->source->items[lexer->pos])) {
                    if (lexer->source->items[lexer->pos] == '\n') {
                         lexer->line++;
                         lexer->col = 1;
                    } else {
                         lexer->col++;
                    }
                    lexer->pos++;
                    continue;
               }

               if (lexer->source->items[lexer->pos] == token->kind) {
                    lexer->pos++;
                    lexer->col++;
                    token->operand++;
               } else {
                    break;
               }
          }
     } else if (token->kind == TOK_JZ) {
          Token next, next_next;
          size_t next_next_pos, next_next_line, next_next_col;

          if (lexer_peekn(*lexer, 0, &next) && lexer_peekn_loc(*lexer, 1, &next_next, &next_next_pos, &next_next_line, &next_next_col)) {
               if ((next.kind == TOK_ADD || next.kind == TOK_SUB) && next.operand == 1 && (next_next.kind == TOK_JNZ)) {
                    lexer->pos = next_next_pos + 1;
                    lexer->line = next_next_line;
                    lexer->col = next_next_col + 1;

                    token->kind = TOK_ZERO;
                    token->operand = 1;
               }
          }
     }

     return true;
}

bool lexer_peek(Lexer lexer, Token *token)
{
     return lexer_peekn(lexer, 0, token);
}

bool lexer_peekn(Lexer lexer, size_t n, Token *token)
{
     if (lexer.line == 0) lexer.line++;
     if (lexer.col == 0)  lexer.col++;

     for (size_t i = 0; i <= n; i++) {
          lexer_skip_non_bf(&lexer);

          if (lexer.pos >= lexer.source->count) return false;

          token->kind = lexer.source->items[lexer.pos];
          token->operand = 1;

          lexer.pos++;
          lexer.col++;

          if (is_bf_cmd_mergeable(token->kind)) {
               while (lexer.pos < lexer.source->count) {
                    if (!is_bf_cmd(lexer.source->items[lexer.pos])) {
                         if (lexer.source->items[lexer.pos] == '\n') {
                              lexer.line++;
                              lexer.col = 1;
                         } else {
                              lexer.col++;
                         }
                         lexer.pos++;
                         continue;
                    }

                    if (lexer.source->items[lexer.pos] == token->kind) {
                         lexer.pos++;
                         lexer.col++;
                         token->operand++;
                    } else {
                         break;
                    }
               }
          }
     }

     return true;
}

bool lexer_peekn_loc(Lexer lexer, size_t n, Token *token, size_t *pos, size_t *line, size_t *col)
{
     if (lexer.line == 0) lexer.line++;
     if (lexer.col == 0)  lexer.col++;

     for (size_t i = 0; i <= n; i++) {
          lexer_skip_non_bf(&lexer);

          if (lexer.pos >= lexer.source->count) return false;

          token->kind = lexer.source->items[lexer.pos];
          token->operand = 1;

          *pos = lexer.pos;
          *line = lexer.line;
          *col = lexer.col;

          lexer.pos++;
          lexer.col++;

          if (is_bf_cmd_mergeable(token->kind)) {
               while (lexer.pos < lexer.source->count) {
                    if (!is_bf_cmd(lexer.source->items[lexer.pos])) {
                         if (lexer.source->items[lexer.pos] == '\n') {
                              lexer.line++;
                              lexer.col = 1;
                         } else {
                              lexer.col++;
                         }
                         lexer.pos++;
                         continue;
                    }

                    if (lexer.source->items[lexer.pos] == token->kind) {
                         lexer.pos++;
                         lexer.col++;
                         token->operand++;
                    } else {
                         break;
                    }
               }
          }
     }

     return true;
}

void lexer_skip_non_bf(Lexer *lexer)
{
     if (lexer->line == 0) lexer->line++;
     if (lexer->col == 0)  lexer->col++;

     while (lexer->pos < lexer->source->count && !is_bf_cmd(lexer->source->items[lexer->pos])) {
          char current = lexer->source->items[lexer->pos];
          if (current == '\n') {
               lexer->line++;
               lexer->col = 1;
          } else {
               lexer->col++;
          }
          lexer->pos++;
     }
}
