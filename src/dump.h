#ifndef DUMP_H
#define DUMP_H 1

#include "struct.h"
#include "common.h"

void print_meta_token(MetaToken token);
void print_meta_lexer(MetaLexer *lexer);
void print_arena(Arena *arena);

#endif
