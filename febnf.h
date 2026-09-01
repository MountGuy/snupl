#ifndef FEBNF_H
#define FEBNF_H 1

#include "common.h"
#include "struct.h"
#include "ebnf_util.h"

void advance_fparser(fParser *parser);
Token *peek_tok_f(fParser *parser);
fExpr *alloc_fexpr(fParser *parser);
int febnf_parser(Lexer *lexer, Token *tokens);
fExpr *fparse_define(fParser *parser);
fExpr *fparse_alter(fParser *parser);
fExpr *fparse_concat(fParser *parser);
fExpr *fparse_primary(fParser *parser);



#endif