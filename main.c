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
    int char_num = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*) malloc(sizeof(char) * (char_num + 10));
    Token *tokens = (Token*) malloc(sizeof(Token) * (char_num + 10));
    
    fread(buf, 1, char_num, fp);
    Lexer lexer;
    ebnf_lexer(buf, &lexer, tokens);

    ebnf_parser(&lexer, tokens);
    return 0;
}