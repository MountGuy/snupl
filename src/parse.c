#include "parse.h"

int debug = 1;

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

void _build_equ(MetaExpr *expr, Grammar *grammar, SetEquBuilder *builder)
{
    Set *first = builder->first, *follow = builder->follow;
    switch (expr->kind)
    {
        case E_ALTER:
        {
            MetaExpr **exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
                _build_equ(exprs[i], grammar, builder);

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int sub_idx = exprs[i]->idx, sup_idx = expr->idx;
                Set *fir1 = first + sub_idx, *fir2 = first + sup_idx;
                append_data(&fir1, 1, &builder->sub_sets);
                append_data(&fir2, 1, &builder->sup_sets);
                if (debug)
                    printf("first %d in first %d\n", sub_idx, sup_idx);
            }
            break;
        }
        case E_CONCAT:
        {
            MetaExpr **exprs = expr->nary.exprs;
            append_data(expr, 1, &builder->concat_exprs);

            for (int i = 0; i < expr->nary.expr_num; i++)
                _build_equ(exprs[i], grammar, builder);

            
            int sub_idx = exprs[0]->idx, sup_idx = expr->idx;
            Set *fir1 = first + sub_idx, *fir2 = first + sup_idx;
            append_data(&fir1, 1, &builder->sub_sets);
            append_data(&fir2, 1, &builder->sup_sets);
            if (debug)
                printf("first %d in first %d\n", sub_idx, sup_idx);
            
            for (int i = 0; i < expr->nary.expr_num - 1; i++)
            {
                int sub_idx = exprs[i + 1]->idx, sup_idx = exprs[i]->idx;
                Set *fir = first + sub_idx, *fol = follow + sup_idx;
                append_data(&fir, 1, &builder->sub_sets);
                append_data(&fol, 1, &builder->sup_sets);
                if (debug)
                    printf("first %d in follow %d\n", sub_idx, sup_idx);
            }
            break;
        }
        case E_OPTION:
        case E_REPEAT:
        {
            _build_equ(expr->unary.expr, grammar, builder);
            // WRITE_OFFSET(builder->first[expr->idx].set, 0);
            break;
        }
        case E_STRING:
        {
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].name == expr->string.value)
                WRITE_OFFSET(builder->first[expr->idx].set, i);
                ;
            break;
        }
        case E_IDENTITY:
        {
            for (int i = 0; i < grammar->tokc_num; i++)
            {
                if (grammar->tokcs[i].type == T_VAR &&  grammar->tokcs[i].name == expr->identity.id)
                    WRITE_OFFSET(builder->first[expr->idx].set, i);
            }
        }
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

    print_grammar(grammar);

    int exact_set_size = grammar->tokc_num;
    int set_size = (exact_set_size + SLB - 1) / SLB * SLB;
    Set *first = malloc(set_num * sizeof(Set));
    Set *follow = malloc(set_num * sizeof(Set));
    ulli *buf = calloc(set_num * 2 * set_size / BYTE_SIZE, 1);
    printf("pad size: %d\n", set_size);
    for (int i = 0; i < set_num; i++)
    {
        first[i].set = buf + set_size / SLB * 2 * i;
        follow[i].set = buf + set_size / SLB * (2 * i + 1);
        first[i].set_size = set_size;
        follow[i].set_size = set_size;
    }

    SetEquBuilder builder = {
        .first = first,
        .follow = follow,
        .sup_sets = init_chunk(sizeof(Set*), 1),
        .sub_sets = init_chunk(sizeof(Set*), 1),
        .concat_exprs = init_chunk(sizeof(MetaExpr*), 1),
    };

    for (int i = 0; i < grammar->def_num; i++)
        _build_equ(grammar->defs[i].expr, grammar, &builder);

    int equ_num = builder.sub_sets.used;
    SetEqu equ = {
        .first = builder.first,
        .follow = builder.follow,
        .sup_sets = fix_chunk(&builder.sup_sets),
        .sub_sets = fix_chunk(&builder.sub_sets),
        .equ_num = equ_num,
        .set_num = set_num,
        .set_size = set_size,
        .exact_set_size = exact_set_size,
    };

    return equ;
}

int apply_equ(SetEqu *equ)
{
    int result = 0;
    for (int i = 0; i < equ->equ_num; i++)
    {
        Set *sub = equ->sub_sets[i], *sup = equ->sup_sets[i];
        for (int j = 0; j < sub->set_size / SLB; j++)
        {
            ulli before = sup->set[j];
            sup->set[j] = before | sub->set[j];
            if (sup->set[j] > before)
                result++;
        }
    }
    return result;
}

int count_one(SetEqu *equ)
{
    int total = 0;
    for (int i = 0; i < equ->set_num; i++)
        for (int j = 0; j < equ->set_size / SLB; j++)
            for (int k = 0; k < SLB; k++)
            {
                if (equ->first[i].set[j] & (((ulli) 1) << k))
                   total++;
                if (equ->follow[i].set[j] & (((ulli) 1) << k))
                   total++;
            }
    return total;
}

int count_one2(Set *first, Set *follow, int set_num)
{
    int total = 0, set_size = first[0].set_size;
    for (int i = 0; i < set_num; i++)
        for (int j = 0; j < set_size / SLB; j++)
            for (int k = 0; k < SLB; k++)
            {
                if (first[i].set[j] & (((ulli) 1) << k))
                   total++;
                if (follow[i].set[j] & (((ulli) 1) << k))
                   total++;
            }
    return total;
}

void solve_firstfollow(Grammar *grammar)
{
    SetEqu equ = build_equ(grammar);
    while(apply_equ(&equ));
    print_setequ_sol(&equ, grammar);
}