#ifndef EBNF_H
#define EBNF_H 1

#include "common.h"
#include "struct.h"
#include "ebnf_util.h"

char *search_asset(Lexer *lexer, char *target);
void ebnf_lexer(char *input, Lexer *lexer, Token *tokens);
int ebnf_parser(Lexer *lexer, Token *tokens);

Expr *parse_define(Parser *parser);
Expr *parse_alter(Parser *parser);
Expr *parse_concat(Parser *parser);
Expr *parse_primary(Parser *parser);


#endif