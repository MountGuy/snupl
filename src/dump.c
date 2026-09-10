#include <stdio.h>

#include "dump.h"

void print_meta_token(MetaToken token)
{
    switch (token.type)
    {
        case M_IDENTITY:
            printf("identity [%d:%d-%d] %s\n", token.line, token.col, token.col + token.len, token.string);
            break;
        case M_OPERATOR:
            printf("operator [%d:%d-%d] %s\n", token.line, token.col, token.col + token.len, token.string);
            break;
        case M_STRING:
            printf("  string [%d:%d-%d] \"%s\"\n", token.line, token.col, token.col + token.len, token.string);
            break;
        default:
            printf("Unexpected token type %d\n", token.type);
            exit(1);
    }
}

void print_meta_lexer(MetaLexer *lexer)
{
    for (int i = 0; i < lexer->token_num; i++)
        print_meta_token(lexer->tokens[i]);
}

void print_grammar(Grammar *grammar)
{
    for (int i = 0; i < grammar->def_num; i++)
    {
        MetaDef *def = grammar->defs + i;
        switch (def->type)
        {
            case D_GRAMMAR:
                printf("grammar [%d] %s := ", i, def->identity);
                break;
            case D_LETTER:
                printf("letters [%d] %s := ", i, def->identity);
                break;
            case D_TERM:
                printf(" term   [%d] %s := ", i, def->identity);
                break;
            default:
                printf("Unexpected def type %d\n", def->type);
                exit(1);
        }
        print_meta_expr(def->expr);
        newline;
    }
}

void print_meta_def(MetaDef *def)
{
    switch (def->type)
    {
        case D_GRAMMAR:
            printf("grammar %s := ", def->identity);
            break;
        case D_LETTER:
            printf("letters %s := ", def->identity);
            break;
        case D_TERM:
            printf(" term   %s := ", def->identity);
            break;
        default:
            printf("Unexpected def type %d\n", def->type);
            exit(1);
    }
    print_meta_expr(def->expr);
    newline;
}

void print_meta_expr(MetaExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_meta_expr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" | ");
            }
            printf(")");
            break;
        case E_CONCAT:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_meta_expr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" , ");
            }
            printf(")");
            break;
        case E_OPTION:
            printf("[");
            print_meta_expr(expr->unary.expr);
            printf("]");
            break;
        case E_REPEAT:
            printf("{");
            print_meta_expr(expr->unary.expr);
            printf("}");
            break;
        case E_STRING:
            printf("\"%s\"", expr->string.value);
            break;
        case E_CRANGE:
            printf("\'%c\'~\'%c\'", expr->crange.lb, expr->crange.ub);
            break;
        case E_IDENTITY:
            printf("%s[%d]", expr->identity.id, expr->identity.idx);
            break;
        default:
            printf("error on print expr: %d\n", expr->kind);
            exit(1);
            break;
    }
}

void print_arena(Arena *arena)
{
    printf("string buffer %d/%d used\n", arena->buffer_used, arena->buffer_max);
    for (int i = 0; i < arena->string_used; i++)
    {
        printf("%3d %s\n", i, arena->strings[i]);
    }
}