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
    printf("print meta lexer\n");
    for (int i = 0; i < lexer->token_num; i++)
        print_meta_token(lexer->tokens[i]);
}

void print_grammar(Grammar *grammar)
{
    printf("number of grammar: %d\n", grammar->def_num);
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

void print_nfa(NFA *nfa)
{
    printf("nfa characters:\n");
    for (int cdx = 0; cdx < nfa->char_num; cdx++)
        printf("[%d] %c-%c\n", cdx, nfa->lbs[cdx], nfa->ubs[cdx]);
    newline;
    printf("nfa ends:\n");
    for (int i = 0; i < nfa->end_num; i++)
        printf("[%d] %s\n", i, nfa->end_names[i]);
    newline;
    printf("trans:\n");
    print_trans(nfa->state_num, nfa->char_num, nfa->trans);
}

void print_trans(int state_num, int char_num, char ***trans)
{
    for (int cdx = 0; cdx < char_num; cdx++)
    {
        for (int i = 0; i < state_num; i++)
        {
            for (int j = 0; j < state_num; j++)
            {
                if (i == j && cdx == 0) printf("x ");
                else printf("%d ", trans[i][j][cdx]);
            }
            newline;
        }
        newline;
    }
}

void print_lexing_result(Lexer *lexer)
{
    for (int i = 0; i < lexer->token_num; i++)
        printf("[%3d] %s\n", i, lexer->tokens[i].string);
}