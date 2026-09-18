#include "meta.h"

char ops[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};
char *S_LPA = ops[0], *S_RPA = ops[1], *S_LBC = ops[2], *S_RBC = ops[3], *S_LBK = ops[4], *S_RBK = ops[5], *S_END = ops[6], *S_EQU = ops[7], *S_ALT = ops[8], *S_CON = ops[9], *S_TIL = ops[10];

MetaToken *peek_tok(MetaParser *parser)
{
    return parser->tokens + parser->cursor;
}

MetaToken *peek_next(MetaParser *parser)
{
    return parser->tokens + parser->cursor + 1;
}

MetaToken *advance_parser(MetaParser *parser)
{
    return parser->tokens + parser->cursor++;
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
            while (cursor[0] != '\n')
                cursor++;
            cursor++;
            line++;
            line_front = cursor;
        }
        else if (cursor[0] == '0' && cursor[1] == 'x')
        {
            char hex_str[2] = { hex_to_int(cursor[2]) * 16 + hex_to_int(cursor[3]), c_null };
            token.string = add_string(hex_str, 1, arena);
            token.type = M_STRING;
            token.line = line;
            token.col = cursor - line_front + 1;
            token.len = 4;
            cursor += 4;
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
    MetaParser parser = {
        .token_num = tokens.used,
        .arena = arena,
    };
    parser.tokens = fix_chunk(&tokens);
    Grammar grammar = parse_define(&parser);
    MetaDef *defs = grammar.defs;
    char **dict = (char**) malloc(sizeof(char*) * (grammar.def_num + 1));
    for (int i = 0; i < grammar.def_num; i++)
        dict[i] = defs[i].identity;
    dict[grammar.def_num] = p_null;

    for (int i = 0; i < grammar.def_num; i++)
        index_identity(dict, defs[i].expr);

    free(dict);

    return grammar;
}

void regist_tok_class(MetaExpr *expr, Chunk *tokcs)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
                regist_tok_class(expr->nary.exprs[i], tokcs);
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

void index_identity(char **dict, MetaExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                index_identity(dict, expr->nary.exprs[i]);
            return;
        case E_OPTION:
        case E_REPEAT:
            index_identity(dict, expr->unary.expr);
            return;
        case E_CRANGE:
        case E_STRING:
            return;
        case E_IDENTITY:
            if (expr->identity.idx == -1)
            {
                for (int i = 0; dict[i]; i++)
                    if (expr->identity.id == dict[i])
                    {
                        expr->identity.idx = i;
                        return;
                    }
            }
            printf("Identity %s is not indexed...\n", expr->identity.id);
            exit(1);
    }
}

Grammar parse_define(MetaParser *parser)
{
    parser->cursor = 0;
    Chunk _defs = init_chunk(sizeof(MetaDef), 1);

    while (parser->cursor < parser->token_num)
    {
        MetaToken *tok_id = advance_parser(parser);
        MetaToken *tok_equ = advance_parser(parser);

        if (!(
            tok_id->type == M_IDENTITY &&
            tok_equ->type == M_OPERATOR &&
            tok_equ->string == S_EQU
        ))
            print_error_mtoken("parse_define equal", tok_equ);
        MetaExpr *expr = parse_alter(parser);
        MetaToken *tok_end = advance_parser(parser);

        if (!(
            tok_end->type == M_OPERATOR &&
            tok_end->string == S_END
        ))
            print_error_mtoken("parse_define end", tok_end);

        MetaDef def;
        def.identity = tok_id->string;
        def.expr = expr;
        if (tok_id->string[0] != '_')
            def.type = D_GRAMMAR;
        else if (tok_id->string[1] != '_')
            def.type = D_TERM;
        else
            def.type = D_LETTER;
        append_data(&def, 1, &_defs);
    }
    Chunk tokcs = init_chunk(sizeof(TokenClass), 1);

    MetaDef *defs = _defs.data;
    for (int i = 0; i < _defs.used; i++)
        if (defs[i].type == D_TERM)
        {
            TokenClass class = {
                .name = defs[i].identity,
                .idx = i,
                .type = T_VAR,
            };
            append_data(&class, 1, &tokcs);
        }

    for (int i = 0; i < _defs.used; i++)
        if (defs[i].type == D_GRAMMAR)
            regist_tok_class(defs[i].expr, &tokcs);

    Grammar grammar;
    grammar.def_num = _defs.used;
    grammar.defs = fix_chunk(&_defs);
    grammar.tokc_num = tokcs.used;
    grammar.tokcs = fix_chunk(&tokcs);

    return grammar;
}

MetaExpr *parse_alter(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr **buffer = (MetaExpr**) malloc(sizeof(MetaExpr*) * parser->token_num);
    MetaExpr *expr;

    while (true)
    {
        buffer[expr_num] = parse_concat(parser);
        MetaToken *token = peek_tok(parser);
        char *string = token->string;
        MType type = token->type;
        expr_num++;
        if ((
            string == S_END ||
            string == S_RPA ||
            string == S_RBC ||
            string == S_RBK) &&
            type == M_OPERATOR
        )
            break;
        else if (type == M_OPERATOR && string == S_ALT)
            advance_parser(parser);
        else
            print_error_mtoken("parse_alter", token);
    }

    if (expr_num == 1)
        expr = buffer[0];
    else
    {
        expr = alloc_expr(parser->arena);
        expr->kind = E_ALTER;
        expr->nary.expr_num = expr_num;
        expr->nary.exprs = alloc_exprs(expr_num, parser->arena);
        memcpy(expr->nary.exprs, buffer, sizeof(MetaExpr*) * expr_num);
    }
    free(buffer);
    return expr;}

MetaExpr *parse_concat(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr **buffer = (MetaExpr**) malloc(sizeof(MetaExpr*) * parser->token_num);
    MetaExpr *expr;

    while (true)
    {
        buffer[expr_num] = parse_primary(parser);
        MetaToken *token = peek_tok(parser);
        char *string = token->string;
        MType type = token->type;
        expr_num++;
        if ((
            string == S_ALT ||
            string == S_END ||
            string == S_RPA ||
            string == S_RBC ||
            string == S_RBK) &&
            type == M_OPERATOR
        )
            break;
        else if (type == M_OPERATOR && string == S_CON)
            advance_parser(parser);
        else
            print_error_mtoken("parse_concat", token);
    }

    if (expr_num == 1)
        expr = buffer[0];
    else
    {
        expr = alloc_expr(parser->arena);
        expr->kind = E_CONCAT;
        expr->nary.expr_num = expr_num;
        expr->nary.exprs = alloc_exprs(expr_num, parser->arena);
        memcpy(expr->nary.exprs, buffer, sizeof(MetaExpr*) * expr_num);
    }
    free(buffer);
    return expr;

}

MetaExpr *parse_primary(MetaParser *parser)
{
    MetaToken *token = advance_parser(parser);
    char *string = token->string;

    switch (token->type)
    {
        case M_STRING:
        {
            MetaToken *next_token = peek_tok(parser);
            if (next_token->type == M_OPERATOR && next_token->string == S_TIL)
            {
                advance_parser(parser);
                MetaToken *end_token = advance_parser(parser);
                MetaExpr *expr = alloc_expr(parser->arena);

                expr->kind = E_CRANGE;
                expr->crange.lb = token->string[0];
                expr->crange.ub = end_token->string[0];
                return expr;
            }
            else
            {
                MetaExpr *expr = alloc_expr(parser->arena);
                expr->kind = E_STRING;
                expr->string.value = string;
                return expr;
            }
        }
        case M_IDENTITY:
        {
            MetaExpr *expr = alloc_expr(parser->arena);
            expr->kind = E_IDENTITY;
            expr->identity.id = string;
            expr->identity.idx = -1;
            return expr;
        }
        case M_OPERATOR:
        {
            if (string == S_LPA || string == S_LBC || string == S_LBK)
            {
                MetaExpr *body = parse_alter(parser);
                MetaToken *next_token = advance_parser(parser);
                char *next_string = next_token->string;

                if (string == S_LPA && next_string == S_RPA)
                    return body;
                if (string == S_LBC && next_string == S_RBC)
                {
                    MetaExpr *expr = alloc_expr(parser->arena);
                    expr->kind = E_REPEAT;
                    expr->unary.expr = body;
                    return expr;
                }
                if (string == S_LBK && next_string == S_RBK)
                {
                    MetaExpr *expr = alloc_expr(parser->arena);
                    expr->kind = E_OPTION;
                    expr->unary.expr = body;
                    return expr;
                }
                print_error_mtoken("parse_primary", token);
            }
        }
        default:
            print_error_mtoken("parse_primary default", token);
    }
    return p_null;
}
