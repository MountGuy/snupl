#ifndef EBNF_H
#define EBNF_H 1

#include "common.h"
#include "struct.h"
#include "ebnf_util.h"

void ebnf_lexer(char *input, Lexer *lexer, Token *tokens);
int ebnf_parser(int tok_num, Token *tokens);

Expr *parse_alt(Parser *parser);
Expr *parse_con(Parser *parser);
Expr *parse_prime(Parser *parser);


#endif