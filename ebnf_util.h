#ifndef EBNF_UTIL
#define EBNF_UTIL 1

#include "common.h"
#include "struct.h"

#define N_GROUPS 3


void advance_parser(Parser *parser);
Token *peek_tok(Parser *parser);
Expr *alloc_expr(Parser *parser);

void print_tokens(int tok_num, Token *tokens);
void print_expr(Expr *expr);
void print_parser(Parser *parser);

#define C_LPAREN  '('
#define C_LBRACE  '{'
#define C_LBRAKET '['
#define C_RPAREN  ')'
#define C_RBRACE  '}'
#define C_RBRAKET ']'
#define C_END     ';'
#define C_DEFINE  '='
#define C_ALTER   '|'
#define C_CONCAT  ','

extern char groups[N_GROUPS][3];
extern char *S_LPAREN, *S_RPAREN, *S_LBRACE, *S_RBRACE, *S_LBRAKET, *S_RBRAKET, *S_END, *S_DEFINE, *S_ALTER, *S_CONCAT;

#endif