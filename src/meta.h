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

Chunk meta_lexing(char *input, Arena *arena);
Grammar meta_parsing(Chunk tokens, Arena *arena);

#endif
