#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "ebnf.h"
#include "ebnf_util.h"


int main(int argv, char *argc[])
{
    if (argv < 2)
    {
        printf("Grammar file path required...\n");
        return 1;
    }

    FILE *fp = fopen(argc[1], "r");
    if (fp == p_null)
    {
        printf("Failed to open grammar file...\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*) malloc(sizeof(char) * file_size);
    Token *tokens = (Token*) malloc(sizeof(Token) * file_size);
    
    fread(buf, 1, file_size, fp);
    int tok_num = ebnf_lexer(buf, tokens);
    print_tokens(tok_num, tokens);
    return 0;
}