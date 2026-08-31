#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "common.h"
#include "ebnf.h"


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

    while (true)
    {
        if ( fgets(buf, 500, fp) == p_null ) break;
        if (strlen(buf) == 1) continue;
        // printf("buf: %s\n", buf);
        buf[strcspn(buf, "\n")] = c_null;

        int tok_num = ebnf_lexer(buf, tokens);
        // printf("tok_num: %d\n", tok_num);
        ebnf_parser(tok_num, tokens);
        // for (int i = 0; i < tok_num; i++)
        // printf("%d %s\n", tokens[i].ttype, tokens[i].token);
        // if (tok_n > 0) printf("======================\n");
    }

    return 0;
}