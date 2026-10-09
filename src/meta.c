#include "meta.h"
#include "character.h"
#include "dump.h"

#define PEEK(p) (p->tokens + p->cursor)
#define POP(p) (p->tokens + p->cursor++)
#define ADVANCE(p) (p->cursor++)
#define IS_1BYTE(cc) (((cc) == CC_ASCII) || ((cc) == CC_NASCII))
#define UTF8_MIN 0x80
#define UTF8_MAX 0xBF

const char lower_t[5] = { UTF8_MIN, UTF8_MIN, UTF8_MIN, UTF8_MIN, UTF8_MIN };
const char upper_t[5] = { UTF8_MAX, UTF8_MAX, UTF8_MAX, UTF8_MAX, UTF8_MAX };

MetaExpr parse_alter(MetaParser *parser);
MetaExpr parse_concat(MetaParser *parser);
MetaExpr parse_primary(MetaParser *parser);
MetaExpr parse_crange(MetaToken *tok_lower, MetaToken *tok_upper, Arena *arena);

char ops[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};
char *S_LPA = ops[0], *S_RPA = ops[1], *S_LBC = ops[2], *S_RBC = ops[3], *S_LBK = ops[4], *S_RBK = ops[5], *S_END = ops[6], *S_EQU = ops[7], *S_ALT = ops[8], *S_CON = ops[9], *S_TIL = ops[10];

void print_error_mtoken(char *comment, MetaToken *token)
{
    printf("Unexpected token %s at [%d:%d-%d] during parsing %s\n", token->string, token->line, token->col, token->col + token->len, comment);
    exit(1);
}

MetaExpr make_crange(uchar lower, uchar upper)
{
    MetaExpr expr = {
        .kind = E_CRANGE,
        .idx = -1,
        .crange.lb = lower,
        .crange.ub = upper,
    };
    return expr;
}

MetaExpr make_string(const char *string)
{
    MetaExpr expr = {
        .kind = E_STRING,
        .idx = -1,
        .string.value = string,
        .string.length = strlen(string),
    };
    return expr;
}

MetaExpr make_nary(int expr_num, MetaExpr *exprs, ExprKind kind, Arena *arena)
{
    if (kind != E_ALTER && kind != E_CONCAT)
        printf("This expr kind is not nary expression\n"), exit(1);
    MetaExpr expr = {
        .kind = kind,
        .idx = -1,
        .nary.expr_num = expr_num,
        .nary.exprs = alloc_mexpr(expr_num, arena),
    };

    if (exprs != p_null)
        memcpy(expr.nary.exprs, exprs, sizeof(MetaExpr) * expr_num);

    return expr;
}

MetaExpr make_unary(ExprKind kind, MetaExpr *body, Arena *arena)
{
    if (kind != E_REPEAT && kind != E_OPTION)
        printf("This expr kind is not unary expression\n"), exit(1);
    MetaExpr expr = {
        .kind = kind,
        .idx = -1,
        .unary.expr = alloc_mexpr(1, arena),
    };

    if (body != p_null)
        *expr.unary.expr = *body;

    return expr;
}

MetaExpr make_identity(const char *string)
{
    MetaExpr expr = {
        .kind = E_IDENTITY,
        .idx = -1,
        .identity.name = string,
        .identity.idx = -1,
    };
    return expr;
}

void regist_tok_class(MetaExpr *expr, Chunk *tokcs)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
                regist_tok_class(expr->nary.exprs + i, tokcs);
            return;
        }
        case E_OPTION:
        case E_REPEAT:
            regist_tok_class(expr->unary.expr, tokcs);
            return;
        case E_STRING:
            TokenClass *data = tokcs->data;
            for (int i = 0; i < tokcs->used; i++)
                if (expr->string.value == data[i].name && data[i].type == T_CONST)
                    return;
            TokenClass class = {
                .name = expr->string.value,
                .idx = tokcs->used,
                .type = T_CONST,
            };
            append_data(&class, 1, tokcs);
            return;
        default:
            return;
    }
}

void index_expr(MetaExpr *expr, Grammar *grammar)
{
    if (expr->idx == -1 && expr->kind != E_IDENTITY)
        expr->idx = grammar->expr_num++;

    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                index_expr(expr->nary.exprs + i, grammar);
            return;
        case E_OPTION:
        case E_REPEAT:
            index_expr(expr->unary.expr, grammar);
            return;
        case E_CRANGE:
        case E_STRING:
            return;
        case E_IDENTITY:
            for (int i = 0; i < grammar->def_num; i++)
                if (expr->identity.name == grammar->defs[i].identity)
                {
                    expr->identity.idx = i;
                    expr->idx = i;
                    return;
                }
            printf("Identity %s is not indexed...\n", expr->identity.name);
            exit(1);
    }
}

Chunk meta_lexing(char *input, Arena *arena)
{
    Chunk tokens = init_chunk(sizeof(MetaToken), 1);

    char *cursor = input, *line_front = input;
    int line = 1;

    while (cursor[0])
    {
        if (cursor[0] == '/' && cursor[1] == '/')
        {
            while ((cursor++)[0] != '\n');
            line++;
            line_front = cursor;
        }
        else if (cursor[0] == '0' && cursor[1] == 'x')
        {
            int hex_len = 0;
            while (is_hex(cursor[hex_len + 2]))
                hex_len++;

            char data[10] = { 0 };
            int string_len = (hex_len + 1) / 2;
            int remain = (hex_len % 2 > 0);

            for (int i = 0; i < hex_len; i++)
                data[(i + remain) / 2] += ((i + remain) % 2? 1 : 16) * hex_to_int(cursor[i + 2]);

            MetaToken token = {
                .string = add_string(data, string_len, arena),
                .type = M_STRING,
                .line = line,
                .col = cursor - line_front + 1,
                .len = string_len,
            };
            append_data(&token, 1, &tokens);
            cursor += hex_len + 2;
        }
        else if (is_identc(cursor[0]))
        {
            int string_len = 0;
            while (is_identc(cursor[string_len]))
                string_len++;
            MetaToken token = {
                .string = add_string(cursor, string_len, arena),
                .type = M_IDENTITY,
                .line = line,
                .col = cursor - line_front + 1,
                .len = string_len,
            };
            append_data(&token, 1, &tokens);
            cursor += string_len;
        }
        else if (cursor[0] == '\"')
        {
            int string_len = 1;
            cursor++;
            while (cursor[string_len] != '\"')
                string_len++;
            MetaToken token = {
                .string = add_string(cursor, string_len, arena),
                .type = M_STRING,
                .line = line,
                .col = cursor - line_front,
                .len = string_len + 2,
            };
            append_data(&token, 1, &tokens);
            cursor += string_len + 1;
        }
        else
        {
            for (int i = 0; i < 11; i++)
            {
                if (cursor[0] == ops[i][0])
                {
                    MetaToken token = {
                        .string = ops[i],
                        .type = M_OPERATOR,
                        .line = line,
                        .col = cursor - line_front + 1,
                        .len = 1,
                    };
                    append_data(&token, 1, &tokens);
                    cursor++;
                    break;
                }
            }
        }

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n')
        {
            if (*cursor == '\n')
            {
                line++;
                line_front = cursor + 1;
            }
            cursor++;
        }
    }

    return tokens;
}

void parse_define(MetaParser *parser)
{
    parser->cursor = 0;

    while (parser->cursor < parser->tok_num)
    {
        MetaToken *tok_id = POP(parser);
        MetaToken *tok_equ = POP(parser);

        if (!(
            tok_id->type == M_IDENTITY &&
            tok_equ->type == M_OPERATOR &&
            tok_equ->string == S_EQU
        ))
            print_error_mtoken("parse_define equal", tok_equ);
        MetaExpr *expr = alloc_mexpr(1, parser->arena);
        *expr = parse_alter(parser);
        MetaToken *tok_end = POP(parser);

        if (!(
            tok_end->type == M_OPERATOR &&
            tok_end->string == S_END
        ))
            print_error_mtoken("parse_define end", tok_end);

        DType type = (tok_id->string[0] != '_'? D_GRAMMAR :
            (tok_id->string[1] != '_'? D_TERM : D_LETTER));

        MetaDef def = {
            .identity = tok_id->string,
            .type = type,
            .expr = expr,
        };
        append_data(&def, 1, &parser->defs);
    }

    int def_num = parser->defs.used;
    MetaDef *defs = parser->defs.data;

    for (int i = 0; i < def_num; i++)
        if (defs[i].type == D_TERM)
        {
            TokenClass class = {
                .name = defs[i].identity,
                .idx = parser->tokcs.used,
                .type = T_VAR,
            };
            append_data(&class, 1, &parser->tokcs);
        }

    for (int i = 0; i < def_num; i++)
        if (defs[i].type == D_GRAMMAR)
            regist_tok_class(defs[i].expr, &parser->tokcs);

}

MetaExpr parse_alter(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr *buffer = malloc(sizeof(MetaExpr) * parser->tok_num);

    while (true)
    {
        buffer[expr_num++] = parse_concat(parser);
        MetaToken *token = PEEK(parser);

        if ((
            token->string == S_END ||
            token->string == S_RPA ||
            token->string == S_RBC ||
            token->string == S_RBK) &&
            token->type == M_OPERATOR
        )
            break;
        else if (token->type == M_OPERATOR && token->string == S_ALT)
            ADVANCE(parser);
        else
            print_error_mtoken("parse_alter", token);
    }

    if (expr_num == 1)
    {
        MetaExpr expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        MetaExpr expr = make_nary(expr_num, buffer, E_ALTER, parser->arena);
        free(buffer);
        return expr;
    }
}

MetaExpr parse_concat(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr *buffer = malloc(sizeof(MetaExpr) * parser->tok_num);

    while (true)
    {
        buffer[expr_num++] = parse_primary(parser);
        MetaToken *token = PEEK(parser);

        if ((
            token->string == S_ALT ||
            token->string == S_END ||
            token->string == S_RPA ||
            token->string == S_RBC ||
            token->string == S_RBK) &&
            token->type == M_OPERATOR
        )
            break;
        else if (token->type == M_OPERATOR && token->string == S_CON)
            ADVANCE(parser);
        else
            print_error_mtoken("parse_concat", token);
    }

    if (expr_num == 1)
    {
        MetaExpr expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        MetaExpr expr = make_nary(expr_num, buffer, E_CONCAT, parser->arena);
        free(buffer);
        return expr;
    }
}

MetaExpr build_branch(const char *lower, const char *upper, int length, Arena *arena)
{
    if (length == 1)
        return make_crange(lower[0], upper[0]);

    MetaExpr case_mid = make_nary(2, p_null, E_CONCAT, arena);
    case_mid.nary.exprs[0] = make_crange(lower[0] + 1, upper[0] - 1);
    case_mid.nary.exprs[1] = build_branch(lower_t, upper_t, length - 1, arena);;

    MetaExpr case_lower = make_nary(2, p_null, E_CONCAT, arena);
    case_lower.nary.exprs[0] = make_crange(lower[0], lower[0]); 
    case_lower.nary.exprs[1] = build_branch(lower + 1, upper_t, length - 1, arena);;

    MetaExpr case_upper = make_nary(2, p_null, E_CONCAT, arena);
    case_upper.nary.exprs[0] = make_crange(upper[0], upper[0]); 
    case_upper.nary.exprs[1] = build_branch(lower_t, upper + 1, length - 1, arena);;

    MetaExpr expr = make_nary(3, p_null, E_ALTER, arena);
    expr.nary.exprs[0] = case_mid;
    expr.nary.exprs[1] = case_lower;
    expr.nary.exprs[2] = case_upper;

    return expr;
}

MetaExpr build_band(const char *lower, const char *upper, int length, Arena *arena)
{
    int idle_d = 0;

    while (lower[idle_d] == upper[idle_d])
        idle_d++;

    if (idle_d == 0)
        return build_branch(lower, upper, length, arena);

    if (idle_d == length)
        return make_string(add_string(lower, idle_d, arena));

    MetaExpr expr = make_nary(2, p_null, E_CONCAT, arena);

    expr.nary.exprs[0] = make_string(add_string(lower, idle_d, arena));
    expr.nary.exprs[1] = build_branch(lower + idle_d, upper + idle_d, length - idle_d, arena);

    return expr;
}

MetaExpr parse_crange(MetaToken *tok_lower, MetaToken *tok_upper, Arena *arena)
{
    const char *str_lower = tok_lower->string, *str_upper = tok_upper->string;
    CharClass cc_lower = det_cc(str_lower), cc_upper = det_cc(str_upper);

    if (cc_lower == CC_NASCII || cc_upper == CC_NASCII)
    {
        if (!IS_1BYTE(cc_lower) || !IS_1BYTE(cc_upper))
            printf("This kind of a crange expression is invalid: %d %d\n", cc_lower, cc_upper), exit(1);
        else
            return make_crange(tok_lower->string[0], tok_upper->string[0]);
    }

    if (cc_lower == cc_upper)
        return build_band(str_lower, str_upper, utf8_bound[cc_lower][0], arena);

    MetaExpr expr = make_nary(cc_upper - cc_lower + 1, p_null, E_ALTER, arena);
        
    for (CharClass cc = cc_lower; cc <= cc_upper; cc++)
    {
        int length = utf8_bound[cc][0];
        char lower[length], upper[length];

        for (int j = 0; j < length; j++)
        {
            lower[j] = (cc == cc_lower? str_lower[j] : utf8_bound[cc][2 * j + 1]);
            upper[j] = (cc == cc_upper? str_upper[j] : utf8_bound[cc][2 * j + 2]);
        }

        expr.nary.exprs[cc - cc_lower] = build_band(lower, upper, length, arena);
    }

    return expr;
}

MetaExpr parse_primary(MetaParser *parser)
{
    MetaToken *token = POP(parser);
    const char *string = token->string;

    switch (token->type)
    {
        case M_STRING:
        {
            MetaToken *next_token = PEEK(parser);
            if (next_token->type == M_OPERATOR && next_token->string == S_TIL)
            {
                ADVANCE(parser);
                MetaToken *end_token = POP(parser);
                return parse_crange(token, end_token, parser->arena);
            }
            else
                return make_string(string);
        }
        case M_IDENTITY:
            return make_identity(string);
        case M_OPERATOR:
        {
            if (string == S_LPA || string == S_LBC || string == S_LBK)
            {
                MetaExpr body = parse_alter(parser);
                MetaToken *next_token = POP(parser);
                const char *next_string = next_token->string;

                if (string == S_LPA && next_string == S_RPA)
                    return body;
                if (string == S_LBC && next_string == S_RBC)
                    return make_unary(E_REPEAT, &body, parser->arena);
                if (string == S_LBK && next_string == S_RBK)
                    return make_unary(E_OPTION, &body, parser->arena);
            }
            print_error_mtoken("parse_primary", token);
        }
    }
    print_error_mtoken("parse_primary default", token);
    exit(1);
}

Grammar meta_parsing(Chunk tokens, Arena *arena)
{
    int tok_num = tokens.used;
    MetaParser parser = {
        .tok_num = tok_num,
        .tokens = fix_chunk(&tokens),
        .defs = init_chunk(sizeof(MetaDef), true),
        .tokcs = init_chunk(sizeof(TokenClass), true),
        .arena = arena,
    };

    parse_define(&parser);

    int def_num = parser.defs.used, tokc_num = parser.tokcs.used;
    Grammar grammar = {
        .def_num = def_num,
        .tokc_num = tokc_num,
        .expr_num = def_num,
        .defs = fix_chunk(&parser.defs),
        .tokcs = fix_chunk(&parser.tokcs),
    };

    MetaDef *defs = grammar.defs;

    for (int i = 0; i < grammar.def_num; i++)
        defs[i].expr->idx = i;

    for (int i = 0; i < grammar.def_num; i++)
        index_expr(defs[i].expr, &grammar);

    return grammar;
}

