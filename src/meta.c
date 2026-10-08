#include "meta.h"
#include "character.h"
#include "dump.h"

#define PEEK(p) (p->tokens + p->cursor)
#define POP(p) (p->tokens + p->cursor++)
#define ADVANCE(p) (p->cursor++)

MetaExpr *parse_alter(MetaParser *parser);
MetaExpr *parse_concat(MetaParser *parser);
MetaExpr *parse_primary(MetaParser *parser);
MetaExpr *cc_expr(char *lower, char *upper, CharClass cc, Arena *arena);
MetaExpr *parse_crange(MetaToken *tok_lower, MetaToken *tok_upper, Arena *arena);

char ops[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};
char *S_LPA = ops[0], *S_RPA = ops[1], *S_LBC = ops[2], *S_RBC = ops[3], *S_LBK = ops[4], *S_RBK = ops[5], *S_END = ops[6], *S_EQU = ops[7], *S_ALT = ops[8], *S_CON = ops[9], *S_TIL = ops[10];

void print_error_mtoken(char *comment, MetaToken *token)
{
    printf("Unexpected token %s at [%d:%d-%d] during parsing %s\n", token->string, token->line, token->col, token->col + token->len, comment);
    exit(1);
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
        MetaExpr *expr = parse_alter(parser);
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

MetaExpr *parse_alter(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr *expr, *buffer = malloc(sizeof(MetaExpr) * parser->tok_num);

    while (true)
    {
        buffer[expr_num++] = *parse_concat(parser);
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
        expr = alloc_mexpr(1, parser->arena);
        *expr = buffer[0];
    }
    else
    {
        expr = alloc_mexpr(expr_num + 1, parser->arena);
        expr->kind = E_ALTER;
        expr->idx = -1;
        expr->nary.expr_num = expr_num;
        expr->nary.exprs = expr + 1;
        memcpy(expr->nary.exprs, buffer, sizeof(MetaExpr) * expr_num);
    }
    free(buffer);
    return expr;
}

MetaExpr *parse_concat(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr *expr, *buffer = malloc(sizeof(MetaExpr) * parser->tok_num);

    while (true)
    {
        buffer[expr_num++] = *parse_primary(parser);
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
        expr = alloc_mexpr(1, parser->arena);
        *expr = buffer[0];
    }
    else
    {
        expr = alloc_mexpr(expr_num + 1, parser->arena);
        expr->kind = E_CONCAT;
        expr->idx = -1;
        expr->nary.expr_num = expr_num;
        expr->nary.exprs = expr + 1;
        memcpy(expr->nary.exprs, buffer, sizeof(MetaExpr) * expr_num);
    }
    free(buffer);
    return expr;
}

MetaExpr *parse_primary(MetaParser *parser)
{
    MetaToken *token = POP(parser);
    char *string = token->string;

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
            {
                MetaExpr *expr = alloc_mexpr(1, parser->arena);
                expr->kind = E_STRING;
                expr->idx = -1;
                expr->string.value = string;
                return expr;
            }
        }
        case M_IDENTITY:
        {
            MetaExpr *expr = alloc_mexpr(1, parser->arena);
            expr->kind = E_IDENTITY;
            expr->idx = -1;
            expr->identity.name = string;
            expr->identity.idx = -1;
            return expr;
        }
        case M_OPERATOR:
        {
            if (string == S_LPA || string == S_LBC || string == S_LBK)
            {
                MetaExpr *body = parse_alter(parser);
                MetaToken *next_token = POP(parser);
                char *next_string = next_token->string;

                if (string == S_LPA && next_string == S_RPA)
                    return body;
                if (string == S_LBC && next_string == S_RBC)
                {
                    MetaExpr *expr = alloc_mexpr(1, parser->arena);
                    expr->kind = E_REPEAT;
                    expr->idx = -1;
                    expr->unary.expr = body;
                    return expr;
                }
                if (string == S_LBK && next_string == S_RBK)
                {
                    MetaExpr *expr = alloc_mexpr(1, parser->arena);
                    expr->kind = E_OPTION;
                    expr->idx = -1;
                    expr->unary.expr = body;
                    return expr;
                }
            }
            print_error_mtoken("parse_primary", token);
        }
        default:
            print_error_mtoken("parse_primary default", token);
    }
    return p_null;
}

MetaExpr *cc_expr(char *_lower, char *_upper, CharClass cc, Arena *arena)
{
    int width = utf8_bound[cc][0];
    uchar *lower = (uchar*) _lower, *upper = (uchar*) _upper;
    MetaExpr *expr = alloc_mexpr(width + 1, arena);

    for (int i = 0; i < width; i++)
    {
        expr[i + 1].kind = E_CRANGE;
        expr[i + 1].idx = -1;
        expr[i + 1].crange.lb = (lower == p_null? utf8_bound[cc][2 * i + 1] : lower[i]);
        expr[i + 1].crange.ub = (upper == p_null? utf8_bound[cc][2 * i + 2] : upper[i]);
    }

    expr[0].kind = E_CONCAT;
    expr[0].idx = -1;
    expr[0].nary.expr_num = width;
    expr[0].nary.exprs = expr + 1;

    return expr;
}

MetaExpr *parse_crange(MetaToken *tok_lower, MetaToken *tok_upper, Arena *arena)
{
    char *str_lower = tok_lower->string, *str_upper = tok_upper->string;
    CharClass cc_lower = det_cc(str_lower), cc_upper = det_cc(str_upper);

    if (cc_lower == CC_NASCII || cc_upper == CC_NASCII)
    {
        if ((cc_lower != CC_ASCII && cc_lower != CC_NASCII) ||
            (cc_upper != CC_ASCII && cc_upper != CC_NASCII))
            {
                printf("This kind of a crange expression is invalid");
                exit(1);
            }
        MetaExpr *expr = alloc_mexpr(1, arena);
        expr->kind = E_CRANGE;
        expr->idx = -1;
        expr->crange.lb = tok_lower->string[0];
        expr->crange.ub = tok_upper->string[0];

        return expr;
    }
    else if (cc_lower == cc_upper)
        return cc_expr(str_lower, str_upper, cc_lower, arena);
    else
    {
        int class_num = cc_upper - cc_lower + 1;
        MetaExpr *expr = alloc_mexpr(class_num + 1, arena);

        expr[1] = *cc_expr(str_lower, p_null, cc_lower, arena);
        expr[class_num] = *cc_expr(p_null, str_upper, cc_upper, arena);
        for (int i = 1; i < class_num - 1; i++)
            expr[i + 1] = *cc_expr(p_null, p_null, cc_lower + i, arena);

        expr[0].kind = E_ALTER;
        expr[0].idx = -1;
        expr[0].nary.expr_num = class_num;
        expr[0].nary.exprs = expr + 1;

        return expr;
    }
}

Chunk meta_lexing(char *input, Arena *arena)
{
    Chunk tokens = init_chunk(sizeof(MetaToken), 1);

    char *cursor = input, *line_front = input;
    int line = 1;

    while (cursor[0])
    {
        MetaToken token = { p_null };
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

            token.string = add_string(data, string_len, arena);
            token.type = M_STRING;
            token.line = line;
            token.col = cursor - line_front + 1;
            token.len = string_len;
            cursor += hex_len + 2;
        }
        else if (is_identc(cursor[0]))
        {
            int string_len = 0;
            while (is_identc(cursor[string_len]))
                string_len++;
            token.string = add_string(cursor, string_len, arena);
            token.type = M_IDENTITY;
            token.line = line;
            token.col = cursor - line_front + 1;
            token.len = string_len;
            cursor += string_len;
        }
        else if (cursor[0] == '\"')
        {
            int string_len = 1;
            cursor++;
            while (cursor[string_len] != '\"')
                string_len++;
            token.string = add_string(cursor, string_len, arena);
            token.type = M_STRING;
            token.line = line;
            token.col = cursor - line_front;
            token.len = string_len + 2;
            cursor += string_len + 1;
        }
        else
        {
            for (int i = 0; i < 11; i++)
            {
                if (cursor[0] == ops[i][0])
                {
                    token.string = ops[i];
                    token.type = M_OPERATOR;
                    token.line = line;
                    token.col = cursor - line_front + 1;
                    token.len = 1;
                    cursor++;
                    break;
                }
            }
        }
        if (token.string != p_null)
            append_data(&token, 1, &tokens);

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

