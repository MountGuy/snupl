#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ebnf_util.h"
#include "ebnf.h"
#include "automata.h"


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

    NFA nfa;
    for (int i = 0; i < gparser.def_num; i++)
        if (gparser.defs[i].identity.str[1] == '_')
            build_NFA(gparser.defs[i].identity.expr, &nfa);

    return 0;
}