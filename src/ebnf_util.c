#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"


char *STR_LPAREN = "(", *STR_RPAREN = ")",
     *STR_LBRACE = "{", *STR_RBRACE = "}",
     *STR_LBRAKET = "[", *STR_RBRAKET = "]",
     *STR_END = ";", *STR_DEFINE = "=",
     *STR_ALTER = "|", *STR_CONCAT = ",",
     *STR_CRANGE = "~";

void resolve_asset(GExpr *expr, SType stype, Asset *asset)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                resolve_asset(expr->nary.exprs[i], stype, asset);
            return;
        case E_STRING:
            expr->string.str = add_asset(expr->string.str, stype, asset);
            return;
        case E_CRANGE:
            char crange[3] = {0};
            crange[0] = expr->crange.start;
            crange[1] = expr->crange.end;
            add_asset(crange, S_CRANGE, asset);
            return;            
        case E_IDENTITY:
            expr->identity.str = add_asset(expr->identity.str, S_IDENTITY, asset);
            return;
        default:
            printf("wtf resolve asset\n");
            exit(1);
    }
}

char *add_asset(char *string, SType stype, Asset *asset)
{
    for (int i = 0; i < asset->asset_num; i++)
        if (strcmp(asset->starts[i], string) == 0 && asset->stypes[i] == stype)
            return asset->starts[i];

    strcpy(asset->top, string);
    
    asset->starts[asset->asset_num] = asset->top;
    asset->stypes[asset->asset_num] = stype;
    asset->top += strlen(string) + 1;
    asset->asset_size += strlen(string) + 1;
    asset->asset_num += 1;

    return asset->starts[asset->asset_num - 1];
}

void set_nary_expr(GExpr *expr, ExprKind kind, GExpr **exprs, int expr_num)
{
    expr->kind = kind;
    expr->nary.exprs = (GExpr**) malloc(sizeof(GExpr*) * expr_num);
    expr->nary.expr_num = expr_num;
    memcpy(expr->nary.exprs, exprs, sizeof(GExpr*) * expr_num);
}

//-----------------------------------------------------------------

GToken *peek_tok(GParser *parser)
{
    return parser->tokens + parser->pos;
}

GToken *peek_next(GParser *parser)
{
    return parser->tokens + parser->pos + 1;
}

GToken *advance_parser(GParser *parser)
{
    GToken *tok = parser->tokens + parser->pos;
    parser->pos++;

    return tok; 
}

GExpr *alloc_expr(GParser *parser)
{
    GExpr *expr = parser->exprs + parser->expr_num;
    parser->expr_num++;

    return expr;
}

//-----------------------------------------------------------------

void print_asset(Asset *asset)
{
    for (int i = 0; i < asset->asset_num; i++)
    {
        switch (asset->stypes[i])
        {
            case S_BASICS:
                printf("[%2d] basic %s\n", i, asset->starts[i]);
                break;
            case S_CRANGE:
                printf("[%2d] crange %c~%c\n", i, asset->starts[i][0], asset->starts[i][1]);
                break;
            case S_GRAMMAR:
                printf("[%2d] grammar %s\n", i, asset->starts[i]);
                break;
            case S_IDENTITY:
                printf("[%2d] identity %s\n", i, asset->starts[i]);
                break;
            default:
                printf("print asset wtf %d %d %s\n", i, asset->stypes[i], asset->starts[i]);
                exit(1);
        }
    }
}

void print_tokens(int tok_num, GToken *tokens)
{
    for (int i = 0; i < tok_num; i++)
    {
        printf("%s ", tokens[i].string);
        if (tokens[i].string == STR_END)
        newline;
    }
    newline;
}

void print_expr(GExpr *expr)
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
        case E_CRANGE:
            printf("\'%c\'~\'%c\'", expr->crange.start, expr->crange.end);
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

void print_parser(GParser *parser)
{
    for (int i = 0; i < parser->def_num; i++)
    {
        GExpr def = parser->defs[i];
        printf("[%3d] %s := ", i, def.identity.str);
        print_expr(def.identity.expr);
        printf("\n");
    }
}

//-----------------------------------------------------------------

void index_identity(GExpr *expr, GParser *parser)
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
        case E_CRANGE:
            break;
        case E_DEFINE:
        default:
            printf("error while index_identity\n");
            exit(1);
    }
}

void unroll_identity(GExpr *expr, GParser *parser)
{
    switch (expr->kind)
    {
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            GExpr *def_body = parser->defs[idx].identity.expr;
            *expr = *def_body;
            if (expr->kind != E_STRING && expr->kind != E_CRANGE)
            {
                expr->nary.exprs = (GExpr**) malloc(sizeof(GExpr*) * expr->nary.expr_num);
                memcpy(expr->nary.exprs, def_body->nary.exprs, sizeof(GExpr*) * expr->nary.expr_num);
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
        case E_CRANGE:
            return;
        default:
            printf("wtf 6 %d\n", expr->kind);
            return;
    }
}

void flatten_expr(GExpr *expr, GParser *parser)
{
    ExprKind kind = expr->kind;
    switch (kind)
    {
        case E_STRING:
        case E_CRANGE:
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
            GExpr **buffer = (GExpr**) malloc(sizeof(GExpr*) * parser->expr_num);
            int expr_num = 0;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                GExpr *tmp = expr->nary.exprs[i];
                flatten_expr(tmp, parser);
                if (tmp->kind == kind)
                {
                    memcpy(buffer + expr_num, tmp->nary.exprs, sizeof(GExpr*) * tmp->nary.expr_num);
                    expr_num += tmp->nary.expr_num;
                }
                else
                {
                    buffer[expr_num] = tmp;
                    expr_num++;
                }
            }

            GExpr **old = expr->nary.exprs;
            set_nary_expr(expr, kind, buffer, expr_num);
            free(old);
            free(buffer);
            
            break;
        }
        default:
            printf("wtf flatten\n");
            exit(1);
    }
}
