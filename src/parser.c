#include "parser.h"
#include "bitop.h"
#include "dump.h"

Expr expr_none = {
    .type = C_NONE,
    .name = p_null,
};

int _null_analysis(MetaExpr *expr, int *can_eps)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int result = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
                if (_null_analysis(expr->nary.exprs + i, can_eps))
                    result = 1;
            return can_eps[expr->idx] = result;
        }
        case E_CONCAT:
        {
            int result = 1;
            for (int i = 0; i < expr->nary.expr_num; i++)
                if (!_null_analysis(expr->nary.exprs + i, can_eps))
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
    append_data(&sub_idx, 1, &equ->sub_idx);
    append_data(&sup_idx, 1, &equ->sup_idx);
    equ->equ_num++;
}

void build_equ(MetaExpr *expr, Grammar *grammar, SetEqu *equ, int *can_eps)
{
    int sn = equ->set_num;

    switch (expr->kind)
    {
        case E_ALTER:
        {
            MetaExpr *exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                build_equ(exprs + i, grammar, equ, can_eps);
                regist_equ(exprs[i].idx, expr->idx, equ);
                regist_equ(expr->idx + sn, exprs[i].idx + sn, equ);
            }
            break;
        }
        case E_CONCAT:
        {
            MetaExpr *exprs = expr->nary.exprs;

            for (int i = 0; i < expr->nary.expr_num; i++)
                build_equ(exprs + i, grammar, equ, can_eps);

            for (int i = 0; i < expr->nary.expr_num - 1; i++)
                regist_equ(exprs[i + 1].idx, exprs[i].idx + sn, equ);

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                regist_equ(exprs[i].idx, expr->idx, equ);

                if (!can_eps[exprs[i].idx])
                    break;
            }

            for (int i = expr->nary.expr_num - 1; i >= 0; i--)
            {
                regist_equ(expr->idx + sn, exprs[i].idx + sn, equ);

                if (!can_eps[exprs[i].idx])
                    break;
            }
            break;
        }
        case E_OPTION:
        case E_REPEAT:
            build_equ(expr->unary.expr, grammar, equ, can_eps);
            regist_equ(expr->idx + sn, expr->unary.expr->idx + sn, equ);
            regist_equ(expr->unary.expr->idx, expr->idx, equ);

            if (expr->kind == E_REPEAT)
                regist_equ(expr->unary.expr->idx, expr->unary.expr->idx + sn, equ);
            break;
        case E_STRING:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].name == expr->string.value)
                    WRITE_OFFSET(equ->sets + equ->offset * expr->idx, i);
            break;
        case E_IDENTITY:
            for (int i = 0; i < grammar->tokc_num; i++)
                if (grammar->tokcs[i].type == T_VAR && grammar->tokcs[i].name == expr->identity.name)
                    WRITE_OFFSET(equ->sets + equ->offset * expr->idx, i);
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
    int offset = equ->offset, equ_num = equ->equ_num;
    int *sub_idx = equ->sub_idx.data, *sup_idx = equ->sup_idx.data;
    int mem_size = equ->set_num * 2 * equ->set_size / BYTE_SIZE;

    memcpy(equ->old_sets, equ->sets, mem_size);

    for (int i = 0; i < equ_num; i++)
        for (int j = 0; j < equ->set_size / SZLIB; j++)
            equ->sets[offset * sup_idx[i] + j] |= equ->sets[offset * sub_idx[i] + j];

    for (int i = 0; i < mem_size / sizeof(ulli); i++)
        if (equ->sets[i] > equ->old_sets[i])
            return 1;
    return 0;
}

FirstFollow solve_ff(Grammar *grammar)
{
    int set_num = grammar->expr_num;
    int set_size = PAD_SIZE(grammar->tokc_num);
    int mem_size = set_num * 2 * set_size / BYTE_SIZE;

    SetEqu equ = {
        .sets = calloc(mem_size, 1),
        .old_sets = calloc(mem_size, 1),
        .set_num = set_num,
        .set_size = set_size,
        .offset = set_size / SZLIB,
        .equ_num = 0,
        .sub_idx = init_chunk(sizeof(int), 1),
        .sup_idx = init_chunk(sizeof(int), 1),
    };

    int *can_eps = calloc(sizeof(int), set_num);
    null_analysis(grammar, can_eps, set_num);

    for (int i = 0; i < grammar->def_num; i++)
        build_equ(grammar->defs[i].expr, grammar, &equ, can_eps);

    while(solve_equ(&equ) > 0);

    free(equ.old_sets), free(can_eps);
    del_chunk(&equ.sub_idx), del_chunk(&equ.sup_idx);

    FirstFollow ff = {
        .sets = equ.sets,
        .set_num = set_num,
        .set_size = set_size,
        .offset = set_size / SZLIB,
        .exact_set_size = grammar->tokc_num,
    };

    return ff;
}

int can_accept(MetaExpr *mexpr, Token *token, FirstFollow *ff)
{
    return READ_OFFSET(ff->sets + ff->offset * mexpr->idx, token->tok_c->idx) > 0;
}

static Token *peek_tok(Parser *parser)
{
    return parser->tokens + parser->cursor;
}

static Token *peek_next(Parser *parser)
{
    return parser->tokens + parser->cursor + 1;
}

static Token *advance_parser(Parser *parser)
{
    return parser->tokens + parser->cursor++;
}

Expr *_parse(MetaExpr *mexpr, Parser *parser, Grammar *grammar)
{
    while (true)
    {
        Token *token = peek_tok(parser);

        if (token->string[0] != '/' || token->string[1] != '/')
            break;
        advance_parser(parser);
    }
    switch (mexpr->kind)
    {
        case E_ALTER:
        {
            Token *token = peek_tok(parser);
            int i1 = -1, i2 = -1;
            for (int i = 0; i < mexpr->nary.expr_num; i++)
                if (can_accept(mexpr->nary.exprs + i, token, parser->ff))
                {
                    if (i1 == -1)
                        i1 = i;
                    else
                        i2 = i;
                }
            if (i2 >= 0)
            {
                Token *next_token = peek_next(parser);
                if (next_token->string[0] == '(' &&
                    strcmp(mexpr->nary.exprs[i2].identity.name, "subroutineCall") == 0)
                    return _parse(mexpr->nary.exprs + i2, parser, grammar);
                else if (
                    strcmp(mexpr->nary.exprs[i1].identity.name, "assignment") == 0 ||
                    strcmp(mexpr->nary.exprs[i1].identity.name, "qualident") == 0)
                    return _parse(mexpr->nary.exprs + i1, parser, grammar);
                else
                {
                    exit(1);
                }
            }
            else if (i1 == -1)
            {
                printf("[%d:%d] ", token->line, token->col);
                printf("Unexpected token here: %s\n", token->string);
                exit(1);
            }
            else
                return _parse(mexpr->nary.exprs + i1, parser, grammar);
        }
        case E_CONCAT:
        {
            Expr *seq = alloc_expr(mexpr->nary.expr_num, parser->arena);
            for (int i = 0; i < mexpr->nary.expr_num; i++)
                seq[i] = *_parse(mexpr->nary.exprs + i, parser, grammar);
            Expr *expr = alloc_expr(1, parser->arena);
            expr->type = C_SEQ;
            expr->name = p_null;
            expr->sequence.exprs = seq;
            expr->sequence.expr_num = mexpr->nary.expr_num;

            return expr;
        }
        case E_OPTION:
        {
            Token *token = peek_tok(parser);
            if (can_accept(mexpr->unary.expr, token, parser->ff))
                return _parse(mexpr->unary.expr, parser, grammar);                
            else
                return &expr_none;
        }
        case E_REPEAT:
        {
            Chunk chunk = init_chunk(sizeof(Expr), true);
            while (can_accept(mexpr->unary.expr, peek_tok(parser), parser->ff))
            {
                Expr *expr = _parse(mexpr->unary.expr, parser, grammar);
                append_data(expr, 1, &chunk);
            }

            int expr_num = chunk.used;
            Expr *exprs = alloc_expr(expr_num, parser->arena);
            memcpy(exprs, chunk.data, sizeof(Expr) * expr_num);
            del_chunk(&chunk);
            
            if (expr_num == 0)
                return &expr_none;
            else if (expr_num == 1)
                return exprs;
            else
            {
                Expr *expr = alloc_expr(1, parser->arena);
                expr->type = C_SEQ;
                expr->name = p_null;
                expr->sequence.exprs = exprs;
                expr->sequence.expr_num = expr_num;
                
                return expr;
            }
        }
        case E_IDENTITY:
        {
            MetaDef def = grammar->defs[mexpr->identity.idx];
            if (def.type == D_GRAMMAR)
            {
                Expr *expr = _parse(def.expr, parser, grammar);
                expr->name = mexpr->identity.name;
                
                return expr;
            }
            else if (def.type == D_TERM)
            {
                Token *token = advance_parser(parser);
                if (token->tok_c->name == mexpr->identity.name && token->tok_c->type == T_VAR)
                {
                    Expr *expr = alloc_expr(1, parser->arena);
                    expr->type = C_TERM;
                    expr->name = def.identity;
                    expr->terminal.token = token;
                    return expr;
                }
                else
                {
                    printf("[%d:%d] ", token->line, token->col);
                    printf("%s was expected but actually got %s\n", mexpr->identity.name, token->string);
                    exit(1);
                }
            }
            printf("Unexpected behavior: parser had gone down into D_LETTER\n");
            exit(1);
        }
        case E_STRING:
        {
            Token *token = advance_parser(parser);
            if (token->string == mexpr->string.value && token->tok_c->type == T_CONST)
            {
                Expr *expr = alloc_expr(1, parser->arena);
                expr->type = C_TERM;
                expr->name = token->tok_c->name;
                expr->terminal.token = token;

                return expr;
            }
            else
            {
                printf("[%d:%d] ", token->line, token->col);
                printf("%s was expected but actually got %s\n", mexpr->string.value, token->string);
                exit(1);
            }
        }
        default:
        {
            printf("Unexpected mexpr kind during _parse\n");
            exit(1);
        }
    }
}

Expr *parse(Chunk *chunk, Grammar *grammar, Arena *arena)
{
    int token_num = chunk->used;
    Token *tokens = fix_chunk(chunk);
    FirstFollow ff = solve_ff(grammar);
    Parser parser = {
        .tokens = tokens,
        .token_num = token_num,
        .ff = &ff,
        .arena = arena,
    };

    return _parse(grammar->defs[0].expr, &parser, grammar);
}

