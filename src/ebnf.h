#ifndef FEBNF_H
#define FEBNF_H 1

#include "common.h"
#include "ebnf_util.h"


Expr *parse_define(Parser *parser);
Expr *parse_alter(Parser *parser);
Expr *parse_concat(Parser *parser);
Expr *parse_primary(Parser *parser);


void ebnf_lexer(Parser *parser);
void ebnf_parser(Parser *parser);
void index_identity(Expr *expr, Parser *parser);

#endif