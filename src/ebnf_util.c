#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"


char *S_LPAREN = "(", *S_RPAREN = ")",
     *S_LBRACE = "{", *S_RBRACE = "}",
     *S_LBRAKET = "[", *S_RBRAKET = "]",
     *S_END = ";", *S_DEFINE = "=",
     *S_ALTER = "|", *S_CONCAT = ",";

char *search_asset(Lexer *lexer, char *target)
{
    for (int i = 0; i < lexer->asset_num; i++)
        if (strcmp(lexer->starts[i], target) == 0)
            return lexer->starts[i];

    strcpy(lexer->top, target);
    
    lexer->starts[lexer->asset_num] = lexer->top;
    lexer->top += strlen(target) + 1;
    lexer->asset_num += 1;

    return lexer->starts[lexer->asset_num - 1];
}

void advance_parser(Parser *parser)
{
    parser->pos++;
}

Token *peek_tok(Parser *parser)
{
    return parser->tokens + parser->pos;
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

void print_fexpr(Expr *expr)
{
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALTER:
        {
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_fexpr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" | ");
            }
            printf(")");
            break;
        }
        case E_CONCAT:
        {
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_fexpr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" , ");
            }
            printf(")");
            break;
        }
        case E_OPTION:
        {
            printf("[");
            print_fexpr(expr->nary.exprs[0]);
            printf("]");
            break;
        }
        case E_REPEAT:
        {
            printf("{");
            print_fexpr(expr->nary.exprs[0]);
            printf("}");
            break;
        }
        case E_STRING:
        {
            printf("\"%s\"", expr->identity.string);
            break;
        }
        case E_IDENTITY:
        {
            printf("%s", expr->identity.string);
            break;
        }
        case E_END:
        {
            printf("\n");
            break;
        }
        case E_DEFINE:
        {
            printf("%s := ", expr->definition.string);
            print_fexpr(expr->definition.expr);
            printf("\n");
            break;
        }
        default:
        {
            printf("error on print expr: %d\n", kind);
            exit(1);
            break;
        }
    }
}