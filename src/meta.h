#ifndef META_H
#define META_H 1

#include "common.h"
#include "struct.h"
#include "arena.h"
#include "character.h"

MetaToken *peek_tok(MetaParser *parser);
MetaToken *peek_next(MetaParser *parser);
MetaToken *advance_parser(MetaParser *parser);
void meta_lexing(MetaLexer *lexer);
Grammar meta_parsing(MetaParser *parser);
void index_identity(char **dict, MetaExpr *expr);
Grammar parse_define(MetaParser *parser);
MetaExpr *parse_alter(MetaParser *parser);
MetaExpr *parse_concat(MetaParser *parser);
MetaExpr *parse_primary(MetaParser *parser);

#endif
