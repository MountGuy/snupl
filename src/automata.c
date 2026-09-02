#include <stdlib.h>

#include "common.h"

int advance_NFA_state(NFA_builder *builder)
{
    int top_state = builder->top_state;
    builder->top_state++;
    return top_state;
}

void build_NFA_expr(int start, int end, Expr *expr, NFA_builder *builder)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int _start = advance_NFA_state(builder), _end = advance_NFA_state(builder);
                build_NFA_expr(_start, _end, expr->nary.exprs[i], builder);
                builder->trans[start][_start][C_EPS] = 1;
                builder->trans[_end][end][C_EPS] = 1;
            }
            break;
        }
        case E_CONCAT:
        {
            int _start, _end = start;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                _start = _end;
                if (i < expr->nary.expr_num - 1)
                    _end = advance_NFA_state(builder);
                else
                    _end = end;
                build_NFA_expr(_start, _end, expr->nary.exprs[i], builder);
            }
            break;
        }
        case E_IDENTITY:
        case E_OPTION:
        case E_REPEAT:
        case E_STRING:
            break;
    }
}
