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
#define C_CRANGE  '~'

char *search_asset(char *string, TType ttype, GParser *parser);
void set_nary_expr(GExpr *expr, ExprKind kind, GExpr **exprs, int expr_num);

GToken *peek_tok(GParser *parser);
GToken *peek_next(GParser *parser);
GToken *advance_parser(GParser *parser);
GExpr *alloc_expr(GParser *parser);

void print_asset(GParser *parser);
void print_tokens(int tok_num, GToken *tokens);
void print_expr(GExpr *expr);
void print_parser(GParser *parser);

void index_identity(GExpr *target, GParser *parser);
void unroll_identity(GExpr *expr, GParser *parser);
void flatten_expr(GExpr *expr, GParser *parser);


extern char *S_LPAREN, *S_RPAREN, *S_LBRACE, *S_RBRACE, *S_LBRAKET, *S_RBRAKET, *S_END, *S_DEFINE, *S_ALTER, *S_CONCAT, *S_CRANGE;

#endif