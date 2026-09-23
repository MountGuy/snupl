#include "parser.h"
#include "bitop.h"
#include "dump.h"

int _null_analysis(MetaExpr *expr, int *can_eps)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int result = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
                if (_null_analysis(expr->nary.exprs[i], can_eps))
                    result = 1;
            return can_eps[expr->idx] = result;
        }
        case E_CONCAT:
        {
            int result = 1;
            for (int i = 0; i < expr->nary.expr_num; i++)
                if (!_null_analysis(expr->nary.exprs[i], can_eps))
                    result = 0;
            return can_eps[expr->idx] = result;
        }
        case E_CRANGE:
        case E_STRING:
            return 0;
        case E_IDENTITY:
            return can_eps[expr->identity.idx];
        case E_OPTION:
        case E_REPEAT:
            _null_analysis(expr->unary.expr, can_eps);
            return can_eps[expr->idx] = 1;
        default:
            printf("Unexpected expr kind during _null_analysis\n");
            exit(1);
    }
}

void null_analysis(Grammar *grammar, int *can_eps, int set_num)
{
    int prev_total = 0, curr_total = 0;
    do {
        prev_total = curr_total;
        curr_total = 0;
        for (int i = 0; i < grammar->def_num; i++)
            curr_total += (can_eps[i] = _null_analysis(grammar->defs[i].expr, can_eps));

    } while (curr_total > prev_total);
}

void regist_equ(int sub_idx, int sup_idx, SetEqu *equ)
{
    append_data(&sub_idx, 1, &equ->sub_sets);
    append_data(&sup_idx, 1, &equ->sup_sets);
}

void build_equ(MetaExpr *expr, Grammar *grammar, SetEqu *equ)
{
    int sn = equ->ff->set_num;

    switch (expr->kind)
    {
        case E_ALTER:
        {
            MetaExpr **exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                build_equ(exprs[i], grammar, equ);
                regist_equ(exprs[i]->idx, expr->idx, equ);
                regist_equ(expr->idx + sn, exprs[i]->idx + sn, equ);
            }
            break;
        }
        case E_CONCAT:
        {
            MetaExpr **exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
                build_equ(exprs[i], grammar, equ);

            for (int i = 0; i < expr->nary.expr_num - 1; i++)
                regist_equ(exprs[i + 1]->idx, exprs[i]->idx + sn, equ);

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                regist_equ(exprs[i]->idx, expr->idx, equ);

                if (!equ->can_eps[exprs[i]->idx])
                    break;
            }

            for (int i = expr->nary.expr_num - 1; i >= 0; i--)
            {
                regist_equ(expr->idx + sn, exprs[i]->idx + sn, equ);

                if (!equ->can_eps[exprs[i]->idx])
                    break;
            }

            break;
        }
        case E_OPTION:
        case E_REPEAT:
            build_equ(expr->unary.expr, grammar, equ);
            regist_equ(expr->idx + sn, expr->unary.expr->idx + sn, equ);
            regist_equ(expr->unary.expr->idx, expr->idx, equ);
            if (expr->kind == E_REPEAT)
                regist_equ(expr->unary.expr->idx, expr->unary.expr->idx + sn, equ);
            break;
        case E_STRING:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].name == expr->string.value)
                    WRITE_OFFSET(equ->ff->sets + equ->ff->offset * expr->idx, i);
            break;
        case E_IDENTITY:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].type == T_VAR && grammar->tokcs[i].name == expr->identity.id)
                    WRITE_OFFSET(equ->ff->sets + equ->ff->offset * expr->idx, i);
            break;
        case E_CRANGE:
            return;
        default:
            printf("Unexpected expr kind during build_equ\n");
            exit(1);
    }
}

int solve_equ(SetEqu *equ)
{
    int result = 0, offset = equ->ff->offset, equ_num = equ->sub_sets.used;
    ulli *sets = equ->ff->sets;
    int *sub_sets = equ->sub_sets.data, *sup_sets = equ->sup_sets.data;

    for (int i = 0; i < equ_num; i++)
    {
        ulli *sub_set = sets + offset * sub_sets[i];
        ulli *sup_set = sets + offset * sup_sets[i];

        for (int j = 0; j < equ->ff->set_size / SZLIB; j++)
        {
            if ((sub_set[j] | sup_set[j]) > sup_set[j])
                result++;
            sup_set[j] |= sub_set[j];
        }
    }
    return result;
}

FirstFollow solve_ff(Grammar *grammar)
{
    int set_num = grammar->expr_num, set_size = PAD_SIZE(grammar->tokc_num);

    FirstFollow ff = {
        .sets = calloc(set_num * 2 * set_size / BYTE_SIZE, 1),
        .set_num = set_num,
        .set_size = set_size,
        .offset = set_size / SZLIB,
        .exact_set_size = grammar->tokc_num,
    };

    SetEqu equ = {
        .ff = &ff,
        .can_eps = calloc(sizeof(int), set_num),
        .sub_sets = init_chunk(sizeof(int), 1),
        .sup_sets = init_chunk(sizeof(int), 1),
    };
    null_analysis(grammar, equ.can_eps, set_num);

    for (int i = 0; i < grammar->def_num; i++)
        build_equ(grammar->defs[i].expr, grammar, &equ);

    while(solve_equ(&equ) > 0);
    free(equ.can_eps), del_chunk(&equ.sub_sets), del_chunk(&equ.sup_sets);

    return ff;
}

void parse(Token *tokens, MetaExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:

        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        case E_IDENTITY:

        case E_STRING:
        {

        }
        case E_CRANGE:
    }
}

