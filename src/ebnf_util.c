#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"


char *S_LPAREN = "(", *S_RPAREN = ")",
     *S_LBRACE = "{", *S_RBRACE = "}",
     *S_LBRAKET = "[", *S_RBRAKET = "]",
     *S_END = ";", *S_DEFINE = "=",
     *S_ALTER = "|", *S_CONCAT = ",";

char *search_asset(char *string, TType ttype, Parser *parser)
{
    for (int i = 0; i < parser->asset_num; i++)
        if (strcmp(parser->starts[i], string) == 0 && parser->asset_types[i] == ttype)
            return parser->starts[i];

    strcpy(parser->top, string);
    
    parser->starts[parser->asset_num] = parser->top;
    parser->asset_types[parser->asset_num] = ttype;
    parser->top += strlen(string) + 1;
    parser->asset_num += 1;

    return parser->starts[parser->asset_num - 1];
}

void set_nary_expr(Expr *expr, ExprKind kind, Expr **exprs, int expr_num)
{
    expr->kind = kind;
    expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
    expr->nary.expr_num = expr_num;
    memcpy(expr->nary.exprs, exprs, sizeof(Expr*) * expr_num);
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



void print_asset(Parser *parser)
{
    for (int i = 0; i < parser->asset_num; i++)
    {
        if (parser->asset_types[i] == T_STRING)
            printf("[%2d] string %s\n", i, parser->starts[i]);
        else if (parser->asset_types[i] == T_IDENTITY)
            continue;
            // printf("[%2d] identi %s\n", i, parser->starts[i]);
        else
            printf("wtf?");
    }
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
            printf("%s[%d]", expr->identity.str, expr->identity.idx);
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



void index_identity(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                index_identity(expr->nary.exprs[i], parser);
            break;
        case E_IDENTITY:
            for (int i = 0; i < parser->def_num; i++)
                if (parser->defs[i].identity.str == expr->identity.str)
                {
                    *expr = parser->defs[i];
                    expr->kind = E_IDENTITY;
                    break;
                }
            break;
        case E_STRING:
            break;
        case E_DEFINE:
        default:
            printf("error while index_identity\n");
            exit(1);
    }
}

void unroll_identity(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            Expr *def_body = parser->defs[idx].identity.expr;
            *expr = *def_body;
            if (expr->kind != E_STRING)
            {
                expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr->nary.expr_num);
                memcpy(expr->nary.exprs, def_body->nary.exprs, sizeof(Expr*) * expr->nary.expr_num);
            }
            unroll_identity(expr, parser);
            return;
        }
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
                unroll_identity(expr->nary.exprs[i], parser);
            return;
        }
        case E_STRING:
            return;
        default:
            printf("wtf? %d\n", expr->kind);
            return;
    }
}

void flatten_expr(Expr *expr, Parser *parser)
{
    ExprKind kind = expr->kind;
    switch (kind)
    {
        case E_STRING:
            break;
        case E_REPEAT:
        case E_OPTION:
        {
            flatten_expr(expr->nary.exprs[0], parser);
            break;
        }
        case E_ALTER:
        case E_CONCAT:
        {
            Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->expr_num);
            int expr_num = 0;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Expr *tmp = expr->nary.exprs[i];
                flatten_expr(tmp, parser);
                if (tmp->kind == kind)
                {
                    memcpy(buffer + expr_num, tmp->nary.exprs, sizeof(Expr*) * tmp->nary.expr_num);
                    expr_num += tmp->nary.expr_num;
                }
                else
                {
                    buffer[expr_num] = tmp;
                    expr_num++;
                }
            }

            Expr **old = expr->nary.exprs;
            set_nary_expr(expr, kind, buffer, expr_num);
            free(old);
            free(buffer);
            
            break;
        }
        default:
            exit(1);
    }
}
