#ifndef EBNF_UTIL
#define EBNF_UTIL 1

#include "common.h"

#define N_GROUPS 3

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

char *search_asset(char *string, TType ttype, Parser *parser);
void set_nary_expr(Expr *expr, ExprKind kind, Expr **exprs, int expr_num);

Token *peek_tok(Parser *parser);
Token *advance_parser(Parser *parser);
Expr *alloc_expr(Parser *parser);

void print_asset(Parser *parser);
void print_tokens(int tok_num, Token *tokens);
void print_expr(Expr *expr);
void print_parser(Parser *parser);

extern char *S_LPAREN, *S_RPAREN, *S_LBRACE, *S_RBRACE, *S_LBRAKET, *S_RBRAKET, *S_END, *S_DEFINE, *S_ALTER, *S_CONCAT;

#endif