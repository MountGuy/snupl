#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "analysis.h"

int null_test(Parser *parser)
{
    int def_num = parser->def_num;
    Boolean *results = (Boolean*) malloc(sizeof(Boolean) * def_num);

    for (int i = 0; i < def_num; i++)
        results[i] = B_VAR;

    int prev_var_num = -1, curr_var_num = 0;

    while (prev_var_num != curr_var_num)
    {
        prev_var_num = curr_var_num;
        curr_var_num = 0;
        for (int i = 0; i < def_num; i++)
        {
            Expr def = parser->defs[i];
            results[i] = expr_null_test(def.identity.expr, results, parser);        
            
            if (results[i] == B_VAR)
                curr_var_num++;
        }
    } 

    for (int i = 0; i < def_num; i++)
    {
        switch (results[i])
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

    free(results);

    return 0;
}

Boolean expr_null_test(Expr *expr, Boolean *results, Parser *parser)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int var_num = 0;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Boolean result = expr_null_test(expr->nary.exprs[i], results, parser);
                
                if (result == B_TRUE)
                    return B_TRUE;
                if (result == B_VAR)
                    var_num++;
            }
            if (var_num > 0)
            return B_VAR;
            else
            return B_FALSE;
        }
        case E_CONCAT:
        {
            int var_num = 0;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Boolean result = expr_null_test(expr->nary.exprs[i], results, parser);
                
                if (result == B_FALSE)
                    return B_FALSE;
                if (result == B_VAR)
                    var_num++;
            }
            if (var_num > 0)
            return B_VAR;
            else
            return B_TRUE;
        }
        case E_OPTION:
        case E_REPEAT:
            return B_TRUE;
        case E_STRING:
            return B_FALSE;
        case E_IDENTITY:
            return results[expr->identity.idx];
        case E_DEFINE:
        default:
            printf("wtf??\n");
            exit(1);
    }
}