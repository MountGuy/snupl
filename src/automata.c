#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "automata.h"
#include "ebnf_util.h"

void sort_resource(CParser *parser)
{
    char *chars = parser->chars;
    for (int i = 0; i < parser->char_num; i++)
    {
        int min_idx = i;
        for (int j = i; j < parser->char_num; j++)
        {
            if (chars[j] < chars[min_idx])
                min_idx = j;
        }
        char tmp = chars[i];
        chars[i] = chars[min_idx];
        chars[min_idx] = tmp;
    }

    char **strings = parser->strings;
    for (int i = 0; i < parser->string_num; i++)
    {
        int min_idx = i;
        for (int j = i; j < parser->string_num; j++)
        {
            if (strcmp(strings[j], strings[min_idx]) < 0)
                min_idx = j;
        }
        char *tmp = strings[i];
        strings[i] = strings[min_idx];
        strings[min_idx] = tmp;
    }
    for (int i = 0; i < parser->string_num; i++)
    {
        int min_idx = i;
        for (int j = i; j < parser->string_num; j++)
        {
            if (strlen(strings[j]) > strlen(strings[min_idx]))
                min_idx = j;
        }
        char *tmp = strings[i];
        strings[i] = strings[min_idx];
        strings[min_idx] = tmp;
    }
}

void add_resource(char *string, NFA *nfa)
{
    int i;
    for (char *c = string; *c; c++)
    {
        for (i = 0; i < nfa->char_num; i++)
            if (nfa->chars[i] == *c)
                break;
        if (i == nfa->char_num)
        {
            nfa->chars[i] = *c;
            nfa->char_num++;
        }
    }
}

void gather_chars(GExpr *expr, NFA *nfa)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                gather_chars(expr->nary.exprs[i], nfa);
            }
            break;
        }
        case E_STRING:
        {
            add_resource(expr->string.str, nfa);
            break;
        }
        case E_IDENTITY:
            break;
        default:
        {
            printf("wtf 3 %d\n", expr->kind);
            exit(1);
        }
    }
}

//-----------------------------------------------------------------

int alloc_NFA_state(NFA *nfa)
{
    int state = nfa->used_state_num;
    nfa->used_state_num++;
    return state;
}

void add_trans(int start, int end, char c, NFA *nfa)
{
    int char_num = nfa->char_num, state_num = nfa->state_num;
    int idx = nfa->char_to_idx[(int) c];
    nfa->trans[(start * state_num  + end) * char_num + idx] = 1;
}

int can_trans(int start, int end, char c, NFA *nfa)
{
    int char_num = nfa->char_num, state_num = nfa->state_num;
    int idx = nfa->char_to_idx[(int) c];
    return nfa->trans[(start * state_num  + end) * char_num + idx];
}

//-----------------------------------------------------------------

int count_state(GExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int count = 1;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                count += count_state(expr->nary.exprs[i]) + 1;
            }
            return count;
        }
        case E_CONCAT:
        {
            int count = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                count += count_state(expr->nary.exprs[i]) + 1;
            }
            return count;
        }
        case E_OPTION:
        case E_REPEAT:
            return count_state(expr->nary.exprs[0]) + 2;
        case E_STRING:
            return strlen(expr->string.str);
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

int build_NFA(GExpr *expr, NFA *nfa)
{
    nfa->char_num = 1;
    nfa->chars = (char*) malloc(sizeof(char) * 300);
    nfa->chars[0] = c_null;
    gather_chars(expr, nfa);

    nfa->state_num = count_state(expr) + 1;
    nfa->used_state_num = 0;
    nfa->char_to_idx[C_EPS] = 0;


    for (int i = 0; i < nfa->char_num; i++)
    {
        char c = nfa->chars[i];
        nfa->char_to_idx[(int) c] = i;
    }
    
    nfa->trans = (int*) malloc(sizeof(int) * nfa->state_num * nfa->state_num * nfa->char_num);

    alloc_NFA_state(nfa);
    _build_NFA(expr, 0, nfa);
    find_reachable(nfa);

    printf("%d %d %d %d\n", nfa->state_num, nfa->used_state_num, nfa->char_num, sizeof(int) * nfa->state_num * nfa->state_num * nfa->char_num / 1024);
    return 0;
}

int _build_NFA(GExpr *expr, int start, NFA *nfa)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int end = alloc_NFA_state(nfa);
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int _start = alloc_NFA_state(nfa);
                int _end = _build_NFA(expr->nary.exprs[i], _start, nfa);
                add_trans(start, _start, C_EPS, nfa);
                add_trans(_end, end, C_EPS, nfa);
            }
            return end;
        }
        case E_CONCAT:
        {
            int prev_end, prev_start = start;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                prev_end = _build_NFA(expr->nary.exprs[i], prev_start, nfa);
                prev_start = alloc_NFA_state(nfa);
                add_trans(prev_end, prev_start, C_EPS, nfa);
            }
            return prev_end;
        }
        case E_OPTION:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa);
            int end = alloc_NFA_state(nfa);
            add_trans(start, body_start, C_EPS, nfa);
            add_trans(body_end, end, C_EPS, nfa);
            add_trans(start, end, C_EPS, nfa);
            return end;
        }
        case E_REPEAT:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa);
            int end = alloc_NFA_state(nfa);
            add_trans(start, body_start, C_EPS, nfa);
            add_trans(body_end, body_start, C_EPS, nfa);
            add_trans(body_end, end, C_EPS, nfa);
            add_trans(start, end, C_EPS, nfa);
            return end;
        }
        case E_STRING:
        {
            int prev_end, prev_start = start;
            for (char *c = expr->string.str; *c; c++)
            {
                prev_end = alloc_NFA_state(nfa);
                add_trans(prev_start, prev_end, *c, nfa);
                prev_start = prev_end;
            }
            return prev_end;
        }
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

void find_reachable(NFA *nfa)
{
    int state_num = nfa->used_state_num;
    int *stack = (int*) malloc(sizeof(int) * state_num), top;
    int *visited = (int*) malloc(sizeof(int) * state_num);
    int curr_state, backed_state = -1, next_state;

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
            if (top > 100) exit(1);
            curr_state = stack[top];
            if (backed_state == -1)
                next_state = 0;
            else
                next_state = backed_state + 1;
            while
            (
                next_state < state_num && (
                can_trans(curr_state, next_state, C_EPS, nfa) == 0 ||
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
            if (visited[i])
                add_trans(state, i, C_EPS, nfa);
        
    }
}

//-----------------------------------------------------------------

void print_reachable(NFA *nfa)
{
    int state_num = nfa->state_num;

    for (int i = 0; i < nfa->used_state_num; i++)
    {
        for (int j = 0; j < nfa->used_state_num; j++)
        {
            if (i == j)
                printf("x ");
            else
                printf("%d ", nfa->trans[(i * state_num  + j) * nfa->char_num]);
        }
        printf("\n");
    }
    printf("\n");
}

int run_NFA(char *string, NFA *nfa)
{
    return 0;
}