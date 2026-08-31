#ifndef EBNF_H
#define EBNF_H 1

#include "common.h"
#include "struct.h"

int ebnf_lexer(char *buf, Token *tokens);
int ebnf_parser(int tok_num, Token *tokens);

Expr *parse_alt(Parser *parser);
Expr *parse_con(Parser *parser);
Expr *parse_prime(Parser *parser);


#endif