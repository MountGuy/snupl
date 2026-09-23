#ifndef META_H
#define META_H 1

#include "common.h"
#include "arena.h"

typedef enum { M_IDENTITY, M_OPERATOR, M_STRING } MType;
typedef struct MetaToken {
    char *string;
    MType type;
    int line, col, len;
} MetaToken;

typedef struct {
    MetaToken *tokens;
    int tok_num, cursor;
    Chunk defs, tokcs;

    Arena *arena;
} MetaParser;

void print_error_mtoken(char *comment, MetaToken *token);
MetaToken *peek_tok(MetaParser *parser);
MetaToken *peek_next(MetaParser *parser);
MetaToken *advance_parser(MetaParser *parser);
Chunk meta_lexing(char *input, Arena *arena);

Grammar meta_parsing(Chunk tokens, Arena *arena);
void regist_tok_class(MetaExpr *expr, Chunk *tokcs);
void index_identity(MetaExpr *expr, Grammar *grammar);
void index_node(MetaExpr *expr, int *counter);

void parse_define(MetaParser *parser);
MetaExpr *parse_alter(MetaParser *parser);
MetaExpr *parse_concat(MetaParser *parser);
MetaExpr *parse_primary(MetaParser *parser);

#endif
