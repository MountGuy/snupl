#include <string.h>
#include <stdio.h>

#include "common.h"
#include "lexer.h"

int lexer(char *str, Token *tokens)
{
    int tok_n = 0;

    for (char *c = str; *c; c++)
    {
        if (*c == ' ') continue;
        else if (is_char(*c))
        {
            char *start = c;
            while (is_char(*(c+1)) || is_digit(*(c+1))) c++;
            strncpy(tokens[tok_n].token, start, c - start + 1);
            tokens[tok_n].token[c - start + 1] = c_null;
            tokens[tok_n].ttype = T_IDENTITY;
            // printf("%s %d %d %d\n", tokens[tok_n].token, tok_n, start - str, c - start + 1);
        }
        else if (*c == '\"')
        {
            char *start = c;
            c++;
            while (*c != '\"') c++;
            strncpy(tokens[tok_n].token, start + 1, c - start - 1);
            tokens[tok_n].token[c - start - 1] = c_null;
            tokens[tok_n].ttype = T_STRING;
            // printf("%s %d %d %d\n", tokens[tok_n].token, tok_n, start + 1 - str, c - start - 1);
        }
        else {
            tokens[tok_n].token[0] = *c;
            tokens[tok_n].token[1] = c_null;
            tokens[tok_n].ttype = T_OPERATOR;
        }
        tok_n++;
    }
    tokens[tok_n].ttype = T_END;

    return tok_n;
}