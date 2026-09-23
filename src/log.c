#include "log.h"
#include "meta.h"

void print_error(char *message)
{
    printf("%s\n", message);
    exit(1);
}

void print_error_mtoken(char *comment, MetaToken *token)
{
    printf("Unexpected token %s at [%d:%d-%d] during parsing %s\n", token->string, token->line, token->col, token->col + token->len, comment);
    exit(1);
}