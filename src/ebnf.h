#ifndef FEBNF_H
#define FEBNF_H 1

#include "common.h"
#include "ebnf_util.h"


GExpr *parse_define(GParser *parser);
GExpr *parse_alter(GParser *parser);
GExpr *parse_concat(GParser *parser);
GExpr *parse_primary(GParser *parser);


void ebnf_lexer(GParser *parser);
void ebnf_parser(GParser *parser);
void index_identity(GExpr *expr, GParser *parser);

#endif