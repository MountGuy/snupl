#ifndef FEBNF_H
#define FEBNF_H 1

#include "common.h"
#include "ebnf_util.h"

void ebnf_lexer(char *input, Lexer *lexer, Token *tokens);
void ebnf_parser(Lexer *lexer, Token *tokens);
void resolve_refer(Expr *target, Parser *parser);

Expr *parse_define(Parser *parser);
Expr *parse_alter(Parser *parser);
Expr *parse_concat(Parser *parser);
Expr *parse_primary(Parser *parser);

#endif