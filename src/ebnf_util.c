#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"


char *S_LPAREN = "(", *S_RPAREN = ")",
     *S_LBRACE = "{", *S_RBRACE = "}",
     *S_LBRAKET = "[", *S_RBRAKET = "]",
     *S_END = ";", *S_DEFINE = "=",
     *S_ALTER = "|", *S_CONCAT = ",";

char *search_asset(Lexer *lexer, char *string, TType ttype)
{
    for (int i = 0; i < lexer->asset_num; i++)
        if (strcmp(lexer->starts[i], string) == 0 && lexer->asset_types[i] == ttype)
            return lexer->starts[i];

    strcpy(lexer->top, string);
    
    lexer->starts[lexer->asset_num] = lexer->top;
    lexer->asset_types[lexer->asset_num] = ttype;
    lexer->top += strlen(string) + 1;
    lexer->asset_num += 1;

    return lexer->starts[lexer->asset_num - 1];
}

void print_asset(Lexer *lexer)
{
    for (int i = 0; i < lexer->asset_num; i++)
    {
        if (lexer->asset_types[i] == T_STRING)
            printf("[%2d] string %s\n", i, lexer->starts[i]);
        else if (lexer->asset_types[i] == T_IDENTITY)
            continue;
            // printf("[%2d] identi %s\n", i, lexer->starts[i]);
        else
            printf("wtf?");
    }
}

Token *peek_tok(Parser *parser)
{
    return parser->tokens + parser->pos;
}

Token *advance_parser(Parser *parser)
{
    Token *tok = parser->tokens + parser->pos;
    parser->pos++;

    return tok; 
}

Expr *alloc_expr(Parser *parser)
{
    Expr *expr = parser->exprs + parser->expr_num;
    parser->expr_num++;

    return expr;
}

void print_tokens(int tok_num, Token *tokens)
{
    for (int i = 0; i < tok_num; i++)
    {
        printf("%p %s\n", tokens[i].string, tokens[i].string);
    }
}

void print_expr(Expr *expr)
{
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALTER:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_expr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" | ");
            }
            printf(")");
            break;
        case E_CONCAT:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_expr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" , ");
            }
            printf(")");
            break;
        case E_OPTION:
            printf("[");
            print_expr(expr->nary.exprs[0]);
            printf("]");
            break;
        case E_REPEAT:
            printf("{");
            print_expr(expr->nary.exprs[0]);
            printf("}");
            break;
        case E_STRING:
            printf("\"%s\"", expr->string.str);
            break;
        case E_IDENTITY:
            printf("%s[%d]", expr->identity.str, expr->identity.id);
            break;
        case E_DEFINE:
            printf("%s := ", expr->identity.str);
            print_expr(expr->identity.expr);
            printf("\n");
            break;
        default:
            printf("error on print expr: %d\n", kind);
            exit(1);
            break;
    }
}

void print_parser(Parser *parser)
{
    for (int i = 0; i < parser->def_num; i++)
    {
        Expr def = parser->defs[i];
        printf("[%3d] %s := ", i, def.identity.str);
        print_expr(def.identity.expr);
        printf("\n");
    }
}