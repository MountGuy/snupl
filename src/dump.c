#include <stdio.h>

#include "dump.h"

void print_meta_token(MetaToken token)
{
    switch (token.type)
    {
        case M_IDENTITY:
            printf("identity [%d:%d-%d] %s\n", token.line, token.col, token.col + token.len, token.string);
            break;
        case M_OPERATOR:
            printf("operator [%d:%d-%d] %s\n", token.line, token.col, token.col + token.len, token.string);
            break;
        case M_STRING:
            printf("  string [%d:%d-%d] \"%s\"\n", token.line, token.col, token.col + token.len, token.string);
            break;
        default:
            printf("Unexpected token type %d\n", token.type);
            exit(1);
    }
}

void print_meta_lexer(MetaLexer *lexer)
{
    for (int i = 0; i < lexer->token_num; i++)
        print_meta_token(lexer->tokens[i]);
}

void print_arena(Arena *arena)
{
    printf("string buffer %d/%d used\n", arena->buffer_used, arena->buffer_max);
    for (int i = 0; i < arena->string_used; i++)
    {
        printf("%3d %s\n", i, arena->strings[i]);
    }
}