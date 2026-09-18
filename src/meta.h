#ifndef META_H
#define META_H 1

#include "common.h"
#include "struct.h"
#include "chunk.h"
#include "arena.h"
#include "character.h"
#include "dump.h"
#include "log.h"

MetaToken *peek_tok(MetaParser *parser);
MetaToken *peek_next(MetaParser *parser);
MetaToken *advance_parser(MetaParser *parser);
Chunk meta_lexing(char *input, Arena *arena);
Grammar meta_parsing(Chunk tokens, Arena *arena);
void regist_tok_class(MetaExpr *expr, Chunk *tokcs);
void index_identity(char **dict, MetaExpr *expr);
Grammar parse_define(MetaParser *parser);
MetaExpr *parse_alter(MetaParser *parser);
MetaExpr *parse_concat(MetaParser *parser);
MetaExpr *parse_primary(MetaParser *parser);

#endif
