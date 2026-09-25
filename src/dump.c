#include "dump.h"
#include "bitop.h"

void print_arena(Arena *arena)
{
    char **strs = arena->string_heads.data;
    for (int i = 0; i < arena->string_num; i++)
    {
        printf("[%d] %s\n", i, strs[i]);
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

void print_meta_lexer(MetaToken *tokens, int token_num)
{
    printf("print meta lexer\n");
    for (int i = 0; i < token_num; i++)
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
    printf("< %d ", expr->idx);
    switch (expr->kind)
    {
        case E_ALTER:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_meta_expr(expr->nary.exprs + i);
                if (i < expr->nary.expr_num - 1)
                    printf(" | ");
            }
            printf(")");
            break;
        case E_CONCAT:
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_meta_expr(expr->nary.exprs + i);
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
            printf("%s[%d]", expr->identity.name, expr->identity.idx);
            break;
        default:
            printf("error on print expr: %d\n", expr->kind);
            exit(1);
            break;
    }
    printf(">");
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
        print_trans(nfa->exact_state_num, nfa->state_num, nfa->char_num, nfa->trans);
    }
    printf("total %d state, %d trimed state, %d char, %d tok class\n", nfa->state_num, nfa->exact_state_num, nfa->char_num, nfa->tokc_num);
}

void print_trans(int exact_state_num, int state_num, int char_num, ulli *trans)
{
    for (int cdx = 0; cdx < char_num; cdx++)
    {
        for (int i = 0; i < exact_state_num; i++)
        {
            for (int j = 0; j < exact_state_num; j++)
            {
                if (i == j && cdx == 0) printf("x");
                else
                {
                    int unit = 8 * sizeof(ulli);
                    int offset = j + state_num * (cdx + char_num * i);
                    int idx = offset / unit, bit = offset % unit;
                    printf("%d", trans[idx] & (((ulli) 1) << bit)? 1 : 0);
                }
            }
            newline;
        }
        newline;
    }
}

void print_lexing_result(Chunk *tok_chunk)
{
    Token *tokens = tok_chunk->data;
    int token_num = tok_chunk->used;
    for (int i = 0; i < token_num; i++)
    {
        Token token = tokens[i];
        printf("[%3d:%2d-%2d] ", token.line, token.col, token.col + token.string_len);
        printf("[%10s:%10s ] %s\n", token.tok_c->type == T_CONST? "grammar" : "value", token.tok_c->name, token.string);
    }
}

void print_binary_vector(ulli *vector, int length)
{
    for (int i = 0; i < length; i++)
        printf("%d", vector[i / 64] & (((ulli) 1)<<(i % 64))? 1 : 0);
    newline;
}

void print_ff_bin(FirstFollow *ff)
{
    int set_size = ff->exact_set_size, offset = ff->offset;
    for (int i = 0; i < ff->set_num; i++)
    {
        printf("first [%d] ", i);
        print_binary_vector(ff->sets + i * offset, set_size);
    }

    newline;

    for (int i = 0; i < ff->set_num; i++)
    {
        printf("follow [%d] ", i);
        print_binary_vector(ff->sets + (i + ff->set_num) * offset, set_size);
    }
}

void print_ff(FirstFollow *ff, Grammar *grammar)
{
    int set_size = ff->exact_set_size, offset = ff->offset;

    for (int i = 0; i < set_size; i++)
    {
        for (int j = 0; j < grammar->def_num; j++)
        {
            if (grammar->defs[j].type != D_GRAMMAR)
                continue;

            if (READ_OFFSET(ff->sets + offset * j, i))
                printf("[%d] %20s is a start of %s\n", i, grammar->tokcs[i].name, grammar->defs[j].identity);
        }
        for (int j = 0; j < grammar->def_num; j++)
        {
            if (grammar->defs[j].type != D_GRAMMAR)
                continue;

            if (READ_OFFSET(ff->sets + offset * (j + ff->set_num), i))
                printf("[%d] %20s is a follow of %s\n", i, grammar->tokcs[i].name, grammar->defs[j].identity);
        }
        
    }
}

void print_ff_tp(FirstFollow *ff, Grammar *grammar)
{
    int set_size = ff->exact_set_size, offset = ff->offset;
    for (int i = 0; i < grammar->def_num; i++)
    {
        if (grammar->defs[i].type != D_GRAMMAR)
            continue;

        ulli *set = ff->sets + offset * i;
        for (int j = 0; j < set_size; j++)
            if (READ_OFFSET(set, j))
                printf("[%d] %20s can starts with    \"%s\"\n", i, grammar->defs[i].identity, grammar->tokcs[j].name);
    }

    for (int i = 0; i < grammar->def_num; i++)
    {
        if (grammar->defs[i].type != D_GRAMMAR)
            continue;

        ulli *set = ff->sets + offset * (i + ff->set_num);
        for (int j = 0; j < set_size; j++)
            if (READ_OFFSET(set, j))
                printf("[%d] %20s can be followed by \"%s\"\n", i, grammar->defs[i].identity, grammar->tokcs[j].name);
    }
}

void print_token(Token token)
{
    printf("token: %20s | class: %10s | class#: %d\n", token.string, token.tok_c->name, token.tok_c->idx);
}

void print_expr(Expr *expr, int depth)
{
    switch (expr->type)
    {
        case C_TERM:
        {
            printf("^%s^ %s ", expr->name, expr->terminal.token->string);
            return;
        }
        case C_SEQ:
        {
            printf("(%s ", expr->name);
            for (int i = 0; i < expr->sequence.expr_num; i++)
                print_expr(expr->sequence.exprs + i, depth + 1);
            printf(")");
            return;
        }
        case C_NONE:
            return;
    }
}
