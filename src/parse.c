#include "parse.h"
#include "dump.h"

void index_node(MetaExpr *expr, int *counter)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                expr->nary.exprs[i]->idx = (*counter)++;
                index_node(expr->nary.exprs[i], counter);
            }
            return;
        case E_OPTION:
        case E_REPEAT:
            expr->unary.expr->idx = (*counter)++;
            index_node(expr->unary.expr, counter);
            return;
        case E_STRING:
        case E_CRANGE:
            return;
        case E_IDENTITY:
            expr->idx = expr->identity.idx;
            return;
        default:
            printf("Unexpected expr kind during count_set\n");
            exit(1);
    }
}

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

int *null_analysis(Grammar *grammar, int set_num)
{
    int *can_eps = calloc(sizeof(int) * set_num, 1);
    int prev_total = 0, curr_total = 0;

    do {
        prev_total = curr_total;
        for (int i = 0; i < grammar->def_num; i++)
            can_eps[i] = _null_analysis(grammar->defs[i].expr, can_eps);
        curr_total = 0;
        for (int i = 0; i < set_num; i++)
            curr_total += can_eps[i];

    } while (curr_total > prev_total);

    return can_eps;
}

void regist_equ(int sub_idx, int sup_idx, SetEquBuilder *builder)
{
    append_data(&sub_idx, 1, &builder->sub_sets);
    append_data(&sup_idx, 1, &builder->sup_sets);
}

void _build_equ(MetaExpr *expr, Grammar *grammar, SetEquBuilder *builder)
{
    int sn = builder->set_num;

    switch (expr->kind)
    {
        case E_ALTER:
        {
            MetaExpr **exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                _build_equ(exprs[i], grammar, builder);
                regist_equ(exprs[i]->idx, expr->idx, builder);
                regist_equ(expr->idx + sn, exprs[i]->idx + sn, builder);
            }
            break;
        }
        case E_CONCAT:
        {
            MetaExpr **exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
                _build_equ(exprs[i], grammar, builder);

            for (int i = 0; i < expr->nary.expr_num - 1; i++)
                regist_equ(exprs[i + 1]->idx, exprs[i]->idx + sn, builder);

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                regist_equ(exprs[i]->idx, expr->idx, builder);

                if (!builder->can_eps[exprs[i]->idx])
                    break;
            }

            for (int i = expr->nary.expr_num - 1; i >= 0; i--)
            {
                regist_equ(expr->idx + sn, exprs[i]->idx + sn, builder);

                if (!builder->can_eps[exprs[i]->idx])
                    break;
            }

            break;
        }
        case E_OPTION:
        case E_REPEAT:
            _build_equ(expr->unary.expr, grammar, builder);
            regist_equ(expr->idx + sn, expr->unary.expr->idx + sn, builder);
            regist_equ(expr->unary.expr->idx, expr->idx, builder);
            if (expr->kind == E_REPEAT)
                regist_equ(expr->unary.expr->idx, expr->unary.expr->idx + sn, builder);
            break;
        case E_STRING:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].name == expr->string.value)
                    WRITE_OFFSET(builder->sets + builder->offset * expr->idx, i);
            break;
        case E_IDENTITY:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].type == T_VAR && grammar->tokcs[i].name == expr->identity.id)
                    WRITE_OFFSET(builder->sets + builder->offset * expr->idx, i);
            break;
        case E_CRANGE:
            return;
        default:
            printf("Unexpected expr kind during _build_equ\n");
            exit(1);
    }
}

SetEqu build_equ(Grammar *grammar)
{
    int set_num = grammar->def_num;

    for (int i = 0; i < grammar->def_num; i++)
        grammar->defs[i].expr->idx = i;

    for (int i = 0; i < grammar->def_num; i++)
        index_node(grammar->defs[i].expr, &set_num);

    int set_size = (grammar->tokc_num + SZLIB - 1) / SZLIB * SZLIB;
    ulli *sets = calloc(set_num * 2 * set_size / BYTE_SIZE, 1);

    SetEquBuilder builder = {
        .sets = sets,
        .set_num = set_num,
        .set_size = set_size,
        .offset = set_size / SZLIB,
        .can_eps = null_analysis(grammar, set_num),
        .sup_sets = init_chunk(sizeof(int), 1),
        .sub_sets = init_chunk(sizeof(int), 1),
    };

    for (int i = 0; i < grammar->def_num; i++)
        _build_equ(grammar->defs[i].expr, grammar, &builder);
    free(builder.can_eps);

    int equ_num = builder.sub_sets.used;

    SetEqu equ = {
        .sets = sets,
        .sup_sets = fix_chunk(&builder.sup_sets),
        .sub_sets = fix_chunk(&builder.sub_sets),
        .equ_num = equ_num,
        .set_num = set_num,
        .set_size = set_size,
        .offset = set_size / SZLIB,
        .exact_set_size = grammar->tokc_num,
    };

    return equ;
}

int apply_equ(SetEqu *equ)
{
    int result = 0;

    for (int i = 0; i < equ->equ_num; i++)
    {
        ulli *sub_set = equ->sets + equ->offset * equ->sub_sets[i];
        ulli *sup_set = equ->sets + equ->offset * equ->sup_sets[i];

        for (int j = 0; j < equ->set_size / SZLIB; j++)
        {
            if ((sup_set[j] | sub_set[j]) > sup_set[j])
                result++;
            sup_set[j] |= sub_set[j];
        }
    }
    return result;
}

void solve_firstfollow(Grammar *grammar)
{
    SetEqu equ = build_equ(grammar);
    while(apply_equ(&equ));
    print_setequ_sol2(&equ, grammar);
}

void parse(Token *tokens, MetaDef *def)
{
    MetaExpr *expr = def->expr;
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_STRING:
        case E_OPTION:
        case E_REPEAT:
        case E_IDENTITY:

        case E_CRANGE:
    }
}

