#include "dump.h"

void print_arena(Arena *arena)
{
    char *str;
    for (int i = 0; i < arena->string_num; i++)
    {
        read_data(&str, i, &arena->string_heads);
        printf("[%d] %s\n", i, str);
    }
    newline;
}

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
    MetaToken *tokens = lexer->tokens;
    printf("print meta lexer\n");
    for (int i = 0; i < lexer->token_num; i++)
        print_meta_token(tokens[i]);
}

void print_grammar(Grammar *grammar)
{
    MetaDef *defs = grammar->defs;
    printf("number of grammar: %d\n", grammar->def_num);
    for (int i = 0; i < grammar->def_num; i++)
    {
        switch (defs[i].type)
        {
            case D_GRAMMAR:
                printf("grammar [%d] %s := ", i, defs[i].identity);
                break;
            case D_LETTER:
                printf("letters [%d] %s := ", i, defs[i].identity);
                break;
            case D_TERM:
                printf(" term   [%d] %s := ", i, defs[i].identity);
                break;
            default:
                printf("Unexpected def type %d\n", defs[i].type);
                exit(1);
        }
        print_meta_expr(defs[i].expr);
        newline;
    }
    printf("number of token class: %d\n", grammar->tokc_num);
    for (int i = 0; i < grammar->tokc_num; i++)
    {
        TokenClass class = grammar->tokcs[i];
        printf("[%d] %s %d\n", i, class.name, class.type);
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

void print_nfa(NFA *nfa, int debug)
{
    printf("nfa characters:\n");
    for (int cdx = 0; cdx < nfa->char_num; cdx++)
        printf("[%d] %c-%c\n", cdx, nfa->lbs[cdx], nfa->ubs[cdx]);
    newline;
    printf("nfa ends:\n");
    for (int i = 0; i < nfa->tokc_num; i++)
        printf("[%d] %s ends with %d\n", i, nfa->tokcs[i].name, nfa->end_states[i]);
    if (debug)
    {
        newline;
        printf("trans:\n");
        print_trans(nfa->trim_state_num, nfa->state_num, nfa->char_num, nfa->trans);
    }
    printf("total %d state, %d trimed state, %d char, %d tok class\n", nfa->state_num, nfa->trim_state_num, nfa->char_num, nfa->tokc_num);
}

void print_trans(int trim_state_num, int state_num, int char_num, unsigned long long int *trans)
{
    for (int cdx = 0; cdx < char_num; cdx++)
    {
        for (int i = 0; i < trim_state_num; i++)
        {
            for (int j = 0; j < trim_state_num; j++)
            {
                if (i == j && cdx == 0) printf("x");
                else
                {
                    int unit = 8 * sizeof(long long int);
                    int offset = j + state_num * (cdx + char_num * i);
                    int idx = offset / unit, bit = offset % unit;
                    // printf("%d %d %d %d %d %d %d \n",i, cdx, j,  offset, idx, bit, trans[idx] & (1 << bit)? 1 : 0);
                    printf("%d", trans[idx] & (((unsigned long long int) 1) << bit)? 1 : 0);
                }
            }
            newline;
        }
        newline;
    }
}

void print_lexing_result(Lexer *lexer)
{
    for (int i = 0; i < lexer->token_num; i++)
    {
        Token token = lexer->tokens[i];
        printf("[%d:%d-%d] ", token.line, token.col, token.col + token.string_len);
        printf("[%s:%s] %s\n", token.tok_c->type == T_CONST? "grammar" : "value", token.tok_c->name, token.string);
    }
}

void print_binary_vector(unsigned long long int *vector, int length)
{
    for (int i = 0; i < length; i++)
        printf("%d", vector[i / 64] & (((unsigned long long int) 1)<<(i % 64))? 1 : 0);
    newline;
}