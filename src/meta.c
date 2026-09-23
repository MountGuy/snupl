#include "meta.h"
#include "character.h"
#include "dump.h"

MetaExpr *parse_alter(MetaParser *parser);
MetaExpr *parse_concat(MetaParser *parser);
MetaExpr *parse_primary(MetaParser *parser);

char ops[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};
char *S_LPA = ops[0], *S_RPA = ops[1], *S_LBC = ops[2], *S_RBC = ops[3], *S_LBK = ops[4], *S_RBK = ops[5], *S_END = ops[6], *S_EQU = ops[7], *S_ALT = ops[8], *S_CON = ops[9], *S_TIL = ops[10];


void print_error_mtoken(char *comment, MetaToken *token)
{
    printf("Unexpected token %s at [%d:%d-%d] during parsing %s\n", token->string, token->line, token->col, token->col + token->len, comment);
    exit(1);
}

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

void indexing(MetaExpr *expr, Grammar *grammar)
{
    if (expr->idx == -1 && expr->kind != E_IDENTITY)
        expr->idx = grammar->expr_num++;

    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                indexing(expr->nary.exprs[i], grammar);
            return;
        case E_OPTION:
        case E_REPEAT:
            indexing(expr->unary.expr, grammar);
            return;
        case E_CRANGE:
        case E_STRING:
            return;
        case E_IDENTITY:
            for (int i = 0; i < grammar->def_num; i++)
                if (expr->identity.id == grammar->defs[i].identity)
                {
                    expr->identity.idx = i;
                    expr->idx = i;
                    return;
                }
            printf("Identity %s is not indexed...\n", expr->identity.id);
            exit(1);
    }
}

void parse_define(MetaParser *parser)
{
    parser->cursor = 0;

    while (parser->cursor < parser->tok_num)
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
                .idx = i,
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
    MetaExpr **buffer = (MetaExpr**) malloc(sizeof(MetaExpr*) * parser->tok_num);
    // MetaExpr *buffer[parser->tok_num];
    MetaExpr *expr;

    while (true)
    {
        buffer[expr_num++] = parse_concat(parser);
        MetaToken *token = peek_tok(parser);

        if ((
            token->string == S_END ||
            token->string == S_RPA ||
            token->string == S_RBC ||
            token->string == S_RBK) &&
            token->type == M_OPERATOR
        )
            break;
        else if (token->type == M_OPERATOR && token->string == S_ALT)
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
        expr->idx = -1;
        expr->nary.expr_num = expr_num;
        expr->nary.exprs = alloc_exprs(expr_num, parser->arena);
        memcpy(expr->nary.exprs, buffer, sizeof(MetaExpr*) * expr_num);
    }
    free(buffer);
    return expr;
}

MetaExpr *parse_concat(MetaParser *parser)
{
    int expr_num = 0;
    MetaExpr **buffer = (MetaExpr**) malloc(sizeof(MetaExpr*) * parser->tok_num);
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
        expr->idx = -1;
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
                expr->idx = -1;
                expr->crange.lb = token->string[0];
                expr->crange.ub = end_token->string[0];
                return expr;
            }
            else
            {
                MetaExpr *expr = alloc_expr(parser->arena);
                expr->kind = E_STRING;
                expr->idx = -1;
                expr->string.value = string;
                return expr;
            }
        }
        case M_IDENTITY:
        {
            MetaExpr *expr = alloc_expr(parser->arena);
            expr->kind = E_IDENTITY;
            expr->idx = -1;
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
                    expr->idx = -1;
                    expr->unary.expr = body;
                    return expr;
                }
                if (string == S_LBK && next_string == S_RBK)
                {
                    MetaExpr *expr = alloc_expr(parser->arena);
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
        indexing(defs[i].expr, &grammar);

    return grammar;
}

