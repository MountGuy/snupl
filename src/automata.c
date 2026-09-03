#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "automata.h"
#include "ebnf_util.h"

void add_resource(char *string, SType stype, CodeParser *c_parser)
{
    int i;
    for (char *c = string; *c; c++)
    {
        for (i = 0; i < c_parser->char_num; i++)
            if (c_parser->chars[i] == *c)
                break;
        if (i == c_parser->char_num)
        {
            c_parser->chars[i] = *c;
            c_parser->char_num++;
        }
    }
    if (stype == S_GRAMMAR)
    {
        for (i = 0; i < c_parser->string_num; i++)
            if (string == c_parser->strings[i])
                break;
        if (i == c_parser->string_num)
        {
            c_parser->strings[c_parser->string_num] = string;
            c_parser->string_num++;
        }
    }
}

void gather_strings(Parser *parser, CodeParser *c_parser)
{
    c_parser->char_num = 0;
    c_parser->chars = (char*) malloc(sizeof(char) * parser->char_num);

    c_parser->string_num = 0;
    c_parser->strings = (char**) malloc(sizeof(char*) * parser->char_num);

    for (int i = 0; i < parser->def_num; i++)
    {
        SType stype;
        if (parser->defs[i].identity.str[0] == '_')
            stype = S_BASIC;
        else
            stype = S_GRAMMAR;
        _gather_strings(parser->defs[i].identity.expr, stype, c_parser);
    }

    c_parser->chars[c_parser->char_num] = c_null;
    printf("characters: %s\n", c_parser->chars);

    for (int i = 0; i < c_parser->string_num; i++)
        printf("[%2d] %s\n", i, c_parser->strings[i]);
}

void _gather_strings(Expr *expr, SType stype, CodeParser *c_parser)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                _gather_strings(expr->nary.exprs[i], stype, c_parser);
            }
            break;
        }
        case E_STRING:
        {
            add_resource(expr->string.str, stype, c_parser);
            break;
        }
        case E_IDENTITY:
            break;
        default:
        {
            printf("wtf %d\n", expr->kind);
            exit(1);
        }
        
    }
}