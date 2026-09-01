#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "struct.h"
#include "analysis.h"

int parser_nulltest(Parser *parser)
{
    int def_num = parser->def_num;
    int *curr_sol = (int*) malloc(sizeof(int) * def_num);
    for (int i = 0; i < def_num; i++) curr_sol[i] = B_VAR;

    int prev_verified_num = 0, curr_verified_num = 0;

    while (B_TRUE)
    {
        curr_verified_num = 0;
        for (int i = 0; i < def_num; i++)
        {
            Expr def = parser->defs[i];
            curr_sol[i] = expr_nulltest(def.identity.expr, curr_sol, parser);        
            
            if (curr_sol[i] != B_VAR) curr_verified_num++;
        }
        if (prev_verified_num == curr_verified_num) break;
        else prev_verified_num = curr_verified_num;
    } 

    for (int i = 0; i < def_num; i++)
    {
        switch (curr_sol[i])
        {
            case B_TRUE:
            printf("[%2d] %30s can be null\n", i, parser->defs[i].identity.str);
            break;
            case B_FALSE:
            printf("[%2d] %30s cannot be null\n", i, parser->defs[i].identity.str);
            break;
            case B_VAR:
            printf("[%2d] %30s not verified \n", i, parser->defs[i].identity.str);
            break;
            default:
            printf("wtf?\n");
            exit(1);
        }
    }

    return 0;
}

int expr_nulltest(Expr *expr, int *cur_sol, Parser *parser)
{
    ExprKind kind = expr->kind;
    
    switch (kind)
    {
        case E_ALTER:
        {
            int has_var = B_FALSE;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int can_null = expr_nulltest(expr->nary.exprs[i], cur_sol, parser);
                if (can_null == B_TRUE) return B_TRUE;
                if (can_null == B_VAR) has_var = B_TRUE;
            }
            if (has_var) return B_VAR;
            else return B_FALSE;
        }
        case E_CONCAT:
        {
            int has_var = B_FALSE;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int can_null = expr_nulltest(expr->nary.exprs[i], cur_sol, parser);
                if (can_null == B_FALSE) return B_FALSE;
                if (can_null == B_VAR) has_var = B_TRUE;
            }
            if (has_var) return B_VAR;
            else return B_TRUE;
        }
        case E_OPTION:
        case E_REPEAT:
            return B_TRUE;
        case E_STRING:
            return B_FALSE;
        case E_IDENTITY:
            return cur_sol[expr->identity.id];
        case E_DEFINE:
        default:
            printf("wtf??\n");
            exit(1);
    }
}