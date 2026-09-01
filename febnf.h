#ifndef FEBNF_H
#define FEBNF_H 1

#include "common.h"
#include "struct.h"
#include "ebnf_util.h"

void ebnf_lexer(char *input, Lexer *lexer, Token *tokens);
void advance_parser(fParser *parser);
Token *peek_tok(fParser *parser);
fExpr *alloc_expr(fParser *parser);
int ebnf_parser(Lexer *lexer, Token *tokens);
fExpr *parse_define(fParser *parser);
fExpr *parse_alter(fParser *parser);
fExpr *parse_concat(fParser *parser);
fExpr *parse_primary(fParser *parser);



#endif