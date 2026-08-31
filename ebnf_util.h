#ifndef EBNF_UTIL
#define EBNF_UTIL 1

#include "common.h"
#include "struct.h"

#define N_GROUPS 3

void print_tokens(int tok_n, Token *tokens);
void advance_parser(Parser *parser);
Token *peek_tok(Parser *parser);
Expr *alloc_arena(Parser *parser);
int parser_end(Parser *parser);
void print_expr(Expr *expr);
void print_parser(Parser *parser);

#define L_PAREN '('
#define L_BRACE '{'
#define L_BRAKET '['
#define R_PAREN ')'
#define R_BRACE '}'
#define R_BRAKET ']'

extern char groups[N_GROUPS][3];
extern char *ebnf_lparen, *ebnf_rparen, *ebnf_lbrace, *ebnf_rbrace, *ebnf_lbraket, *ebnf_rbraket, *ebnf_end, *ebnf_def, *ebnf_alt, *ebnf_con;

#endif