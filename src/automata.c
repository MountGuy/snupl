#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "automata.h"
#include "ebnf_util.h"

#define NFA_has_c(nfa, c) ((nfa)->char_to_idx[(int) (c)] != -1)

void regist_char(char left, char right, NFA *nfa)
{
    int char_num = nfa->char_num;
    for (int i = 0; i < char_num; i++)
        if (left == nfa->l_chars[i] && right == nfa->r_chars[i])
            return;

    nfa->l_chars[char_num] = left;
    nfa->r_chars[char_num] = right;
    nfa->char_num++;
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
                gather_chars(expr->nary.exprs[i], nfa);
            break;
        }
        case E_STRING:
        {
            for (char *c = expr->string.str; *c; c++)
                regist_char(*c, *c, nfa);
            break;
        }
        case E_CRANGE:
        {
            regist_char(expr->crange.start, expr->crange.end, nfa);
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

int find_char(char left, char right, NFA *nfa)
{
    for (int i = 0; i < nfa->char_num; i++)
        if (left == nfa->l_chars[i] && right == nfa->r_chars[i])
            return i;
    printf("wtf findchar\n");
    exit(1);
}

//-----------------------------------------------------------------

int alloc_NFA_state(NFA *nfa)
{
    int state = nfa->used_state_num;
    nfa->used_state_num++;
    return state;
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

int count_char(GExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int count = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_char(expr->nary.exprs[i]);
            return count;
        }
        case E_CONCAT:
        {
            int count = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_char(expr->nary.exprs[i]);
            return count;
        }   
        case E_OPTION:
        case E_REPEAT:
            return count_char(expr->nary.exprs[0]);
        case E_STRING:
            return strlen(expr->string.str);
        case E_CRANGE:
            return 1;
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

void build_NFA(GExpr *expr, NFA *nfa)
{
    int char_num = count_char(expr) + 1;
    nfa->l_chars = (char*) malloc(sizeof(char) * char_num);
    nfa->r_chars = (char*) malloc(sizeof(char) * char_num);
    nfa->l_chars[0] = c_null;
    nfa->r_chars[0] = c_null;
    nfa->char_num = 1;
    gather_chars(expr, nfa);

    nfa->state_num = count_state(expr) + 1;
    nfa->used_state_num = 0;

    nfa->trans = (int*) calloc(nfa->state_num * nfa->state_num * nfa->char_num, sizeof(int));
    nfa->start = alloc_NFA_state(nfa);
    nfa->end = _build_NFA(expr, nfa->start, nfa);

    nfa->visiting = (int*) malloc(sizeof(int) * nfa->state_num);
    nfa->visiting_new = (int*) malloc(sizeof(int) * nfa->state_num);

    find_reachable(nfa);
    absurb_eps(nfa);
    
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
                NFA_TRANS(start, _start, I_EPS, nfa) = B_TRUE;
                NFA_TRANS(_end, end, I_EPS, nfa) = B_TRUE;
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
                NFA_TRANS(prev_end, prev_start, I_EPS, nfa) = B_TRUE;
            }
            return prev_end;
        }
        case E_OPTION:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, end, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(start, end, I_EPS, nfa) = B_TRUE;
            return end;
        }
        case E_REPEAT:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, end, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(start, end, I_EPS, nfa) = B_TRUE;
            return end;
        }
        case E_STRING:
        {
            int prev_end, prev_start = start;
            for (char *c = expr->string.str; *c; c++)
            {
                int i = find_char(*c, *c, nfa);
                prev_end = alloc_NFA_state(nfa);
                NFA_TRANS(prev_start, prev_end, i, nfa) = B_TRUE;
                prev_start = prev_end;
            }
            return prev_end;
        }
        case E_CRANGE:
        {
            char left = expr->crange.start, right = expr->crange.end;
            int i = find_char(left, right, nfa);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, end, i, nfa) = B_TRUE;
            return end;
        }
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

//-----------------------------------------------------------------

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

int run_NFA(char *string, NFA *nfa)
{
    int state_num = nfa->used_state_num;
    int char_num = nfa->char_num;
    int *visiting = (int*) malloc(sizeof(int) * state_num);
    int *visiting_new = (int*) malloc(sizeof(int) * state_num);
    int *valid_char = (int*) malloc(sizeof(int) * char_num);

    for (int i = 0; i < state_num; i++)
        visiting[i] = NFA_TRANS(nfa->start, i, I_EPS, nfa);

    for (char *c = string; *c; c++)
    {
        for (int i = 0; i < char_num; i++)
            valid_char[i] = nfa->l_chars[i] <= *c && *c <= nfa->r_chars[i];

        for (int i = 0; i < state_num; i++)
            visiting_new[i] = 0;

        for (int i = 0; i < state_num; i++)
        {
            if (!visiting[i])
                continue;
            for (int j = 0; j < state_num; j++)
                for (int k = 0; k < char_num; k++)
                    visiting_new[j] |= NFA_TRANS(i, j, k, nfa) && valid_char[k];
        }
        int *tmp = visiting;
        visiting = visiting_new;
        visiting_new = tmp;
    }

    int success = visiting[nfa->end];

    free(visiting);
    free(visiting_new);
    free(valid_char);

    return success;
}

void init_NFA_run(NFA *nfa)
{
    for (int i = 0; i < nfa->state_num; i++)
        nfa->visiting[i] = NFA_TRANS(nfa->start, i, I_EPS, nfa);
}

int step_NFA(char c, NFA *nfa)
{
    int state_num = nfa->state_num, char_num = nfa->char_num;
    for (int i = 0; i < state_num; i++)
        nfa->visiting_new[i] = 0;

    for (int k = 0; k < char_num; k++)
    {
        if (!(nfa->l_chars[k] <= c && c <= nfa->r_chars[k]))
            continue;
        for (int i = 0; i < state_num; i++)
        {
            if (!nfa->visiting[i])
                continue;
            for (int j = 0; j < state_num; j++)
                nfa->visiting_new[j] |= NFA_TRANS(i, j, k, nfa);
        }
    }
    
    int *tmp = nfa->visiting;
    nfa->visiting = nfa->visiting_new;
    nfa->visiting_new = tmp;
    
    int total = 0;
    for (int i = 0; i < state_num; i++)
        total += nfa->visiting[i];

    return total > 0;

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
                printf("x");
            else
                printf("%d", nfa->trans[(i * state_num  + j) * nfa->char_num]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_NFA(NFA *nfa)
{
    printf("state num: %d/%d\n", nfa->used_state_num, nfa->state_num);
    printf("chars: \n");
    for (int i = 0; i < nfa->char_num; i++)
    {
        printf("%d [\"%c\" ~ \"%c\"]\n", i, nfa->l_chars[i], nfa->r_chars[i]);
    }
    printf("start: %d, end: %d\n", nfa->start, nfa->end);
}


