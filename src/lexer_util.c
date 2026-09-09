#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer_util.h"


int find_char(char lb, char ub, Lexer *lexer)
{
    for (int i = 0; i < lexer->char_num; i++)
        if (lb == lexer->char_lbs[i] && ub == lexer->char_ubs[i])
            return i;
    printf("wtf findchar %c %c\n", lb, ub);
    exit(1);
}

int count_state(GExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int count = 1;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_state(expr->nary.exprs[i]) + 1;
            return count;
        }
        case E_CONCAT:
        {
            int count = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_state(expr->nary.exprs[i]) + 1;
            return count;
        }   
        case E_OPTION:
        case E_REPEAT:
            return count_state(expr->nary.exprs[0]) + 2;
        case E_STRING:
            return strlen(expr->string.str);
        case E_CRANGE:
            return 1;
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

int alloc_NFA_state(NFA *nfa)
{
    int state = nfa->used_state_num;
    nfa->used_state_num++;
    return state;
}

void find_reachable(NFA *nfa)
{
    int state_num = nfa->used_state_num;
    int *stack = (int*) malloc(sizeof(int) * state_num);
    int *visited = (int*) malloc(sizeof(int) * state_num);
    int curr_state, backed_state = -1, next_state, top;

    for (int state = 0; state < state_num; state++)
    {
        for (int i = 0; i < state_num; i++)
            visited[i] = 0;
        top = 0;
        stack[0] = state;
        visited[state] = 1;
        backed_state = -1;

        while (top >= 0)
        {
            curr_state = stack[top];
            if (backed_state == -1)
                next_state = 0;
            else
                next_state = backed_state + 1;
            while
            (
                next_state < state_num && (
                !NFA_TRANS(curr_state, next_state, I_EPS, nfa) ||
                visited[next_state] == 1
            ))
            next_state++;

            if (next_state == state_num)
            {
                backed_state = curr_state;
                curr_state = stack[top];
                top--;
                continue;
            }
            else
            {
                top++;
                stack[top] = next_state;
                backed_state = -1;
                visited[next_state] = 1;
                continue;
            }
        }

        for (int i = 0; i < state_num; i++)
            NFA_TRANS(state, i, I_EPS, nfa) |= visited[i];
    }
}

void absurb_eps(NFA *nfa)
{
    int state_num = nfa->state_num, char_num = nfa->char_num;
    
    for (int i = 0; i < state_num; i++)
        for (int j = 0; j < state_num; j++)
            for (int k = 0; k < char_num; k++)
                if (NFA_TRANS(i, j, k, nfa))
                    for (int l = 0; l < state_num; l++)
                        NFA_TRANS(i, l, k, nfa) |= NFA_TRANS(j, l, I_EPS, nfa);
}

//-----------------------------------------------------------------

void _regist_char(char lb, char ub, Lexer *lexer)
{
    int char_num = lexer->char_num;
    for (int i = 0; i < char_num; i++)
        if (lb == lexer->char_lbs[i] && ub == lexer->char_ubs[i])
            return;

    lexer->char_lbs[char_num] = lb;
    lexer->char_ubs[char_num] = ub;
    lexer->char_num++;
}

void regist_char(Asset *asset, Lexer *lexer)
{
    lexer->char_lbs = (char*) malloc(sizeof(char*) * asset->asset_size);
    lexer->char_ubs = (char*) malloc(sizeof(char*) * asset->asset_size);
    lexer->char_num = 1;

    for (int i = 0; i < asset->asset_num; i++)
    {
        char *string = asset->starts[i];
        SType stype = asset->stypes[i];

        switch (stype)
        {
            case S_BASICS:
                for (char *c = string; *c; c++)
                    _regist_char(*c, *c, lexer);
            break;
            case S_CRANGE:
                _regist_char(string[0], string[1], lexer);
                break;
            case S_GRAMMAR:
                for (char *c = string; *c; c++)
                    _regist_char(*c, *c, lexer);
                break;
            case S_IDENTITY:
                break;
            default:
                printf("wtf lexing\n");
                exit(1);
        }
    }
}

