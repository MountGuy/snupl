#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ebnf_util.h"
#include "ebnf.h"
#include "lexer.h"


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
    fread(buf, 1, char_num, fp);
    buf[char_num] = c_null;
    
    GParser gparser;
    gparser.input = buf;
    ebnf_lexer(&gparser);
    ebnf_parser(&gparser);

    fclose(fp);
    fp = fopen(argc[2], "r");
    if (fp == p_null)
    {
        printf("Failed to open code file...\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    char_num = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = (char*) malloc(sizeof(char) * (char_num + 10));
    fread(buf, 1, char_num, fp);
    buf[char_num] = c_null;

    Lexer lexer;
    lexer.input = buf;
    lexer.strings = (char**) malloc(sizeof(char*) * char_num);
    lexer.string_num = 0;

    lexing(&gparser, &lexer);

    return 0;
}