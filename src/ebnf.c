#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"
#include "analysis.h"
#include "lexer.h"

GExpr *parse_define(GParser *parser)
{
    int def_num = 0, lexterm_num = 0;

    while (parser->pos < parser->tok_num)
    {
        GToken *tok_id = advance_parser(parser);
        GToken *tok_def = advance_parser(parser);

        if (!(
            tok_id->ttype == T_IDENTITY &&
            tok_def->ttype == T_OPERATOR &&
            tok_def->string == STR_DEFINE
        ))
        {
            printf("definition format error\n");
            exit(1);
        }

        GExpr *expr = parse_alter(parser);
        GToken *tok_end = advance_parser(parser);

        if (!(
            tok_end->ttype == T_OPERATOR &&
            tok_end->string == STR_END
        ))
        {
            printf("definition format error\n");
            exit(1);
        }

        parser->defs[def_num].kind = E_DEFINE;
        parser->defs[def_num].identity.idx = def_num;
        parser->defs[def_num].identity.str = tok_id->string;
        parser->defs[def_num].identity.expr = expr;
        def_num++;
        if (tok_id->string[0] == '_' && tok_id->string[1] != '_')
            lexterm_num++;
    }

    parser->def_num = def_num;
    parser->lexterm_num = lexterm_num;

    return parser->defs;
}

GExpr *parse_alter(GParser *parser)
{
    int expr_num = 0;
    GExpr **buffer = (GExpr**) malloc(sizeof(GExpr*) * parser->tok_num);

    while (B_TRUE)
    {
        buffer[expr_num] = parse_concat(parser);
        GToken *token = peek_tok(parser);
        char *string = peek_tok(parser)->string;
        TType ttype = token->ttype;
        expr_num++;
        
        if ((
            string == STR_END ||
            string == STR_RPAREN ||
            string == STR_RBRACE ||
            string == STR_RBRAKET) &&
            ttype == T_OPERATOR
        )
            break;
        else if (ttype == T_OPERATOR && string == STR_ALTER)
        {
            advance_parser(parser);
            continue;
        }
        else
        {
            printf("error 1\n");
            exit(1);
        }
    }
    if (expr_num == 1)
    {
        GExpr *expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        GExpr *expr = alloc_expr(parser);
        set_nary_expr(expr, E_ALTER, buffer, expr_num);
        free(buffer);
        return expr;
    }
}

GExpr *parse_concat(GParser *parser)
{
    int expr_num = 0;
    GExpr **buffer = (GExpr**) malloc(sizeof(GExpr*) * parser->tok_num);

    while (B_TRUE)
    {
        buffer[expr_num] = parse_primary(parser);
        GToken *token = peek_tok(parser);
        char *string = peek_tok(parser)->string;
        TType ttype = token->ttype;
        expr_num++;

        if ((
            string == STR_ALTER ||
            string == STR_END ||
            string == STR_RPAREN ||
            string == STR_RBRACE ||
            string == STR_RBRAKET) &&
            ttype == T_OPERATOR
        )
            break;
        else if (ttype == T_OPERATOR && string == STR_CONCAT)
        {
            advance_parser(parser);
            continue;
        }
        else
        {
            printf("error 2 %s %d\n", string, ttype);
            exit(1);
        }
    }
    if (expr_num == 1)
    {
        GExpr *expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        GExpr *expr = alloc_expr(parser);
        set_nary_expr(expr, E_CONCAT, buffer, expr_num);
        free(buffer);
        return expr;
    }
}

GExpr *parse_primary(GParser *parser)
{
    GToken *token = advance_parser(parser);
    TType ttype = token->ttype;
    char *string = token->string;
    GExpr *expr;

    switch (ttype)
    {
        case T_STRING:
        {
            GToken *next_token = peek_tok(parser);
            if (next_token->ttype == T_OPERATOR && next_token->string == STR_CRANGE)
            {
                advance_parser(parser);
                GToken *end_token = advance_parser(parser);
                expr = alloc_expr(parser);
                expr->kind = E_CRANGE;
                expr->crange.start = token->string[0];
                expr->crange.end = end_token->string[0];
                return expr;
            }
            else
            {
                expr = alloc_expr(parser);
                expr->kind = E_STRING;
                expr->string.str = string;
                return expr;
            }
        }
        case T_IDENTITY:
            expr = alloc_expr(parser);
            expr->kind = E_IDENTITY;
            expr->identity.idx = 0;
            expr->identity.str = string;
            expr->identity.expr = p_null;
            return expr;
        case T_OPERATOR:
            if (
                string == STR_LPAREN ||
                string == STR_LBRACE ||
                string == STR_LBRAKET
            )
            {
                GExpr *body_expr = parse_alter(parser);
                GToken *next_token = advance_parser(parser);
                char *next_string = next_token->string;

                if (string == STR_LPAREN && next_string == STR_RPAREN)
                    return body_expr;
                else if (
                    (string == STR_LBRACE && next_string == STR_RBRACE) ||
                    (string == STR_LBRAKET && next_string == STR_RBRAKET)
                )
                {
                    GExpr *expr = alloc_expr(parser);
                    set_nary_expr(expr, (string == STR_LBRACE? E_REPEAT: E_OPTION), &body_expr, 1);

                    return expr;                    
                }
                else {   
                    printf("unexpected parsing: parse primary, %s %s\n", string, peek_tok(parser)->string);
                    exit(1);
                }
            }

            printf("unexpected parsing: parse primary, %s %s\n", string, peek_tok(parser)->string);
            exit(1);
        default:
            printf("unexpected parsing: parse primary\n");
            exit(1);
    }
}

void init_asset(int char_num, int tok_num, Asset *asset)
{
    asset->assets = (char*) malloc(sizeof(char) * (char_num + 10));
    asset->starts = (char**) malloc(sizeof(char*) * (tok_num + 10));
    asset->top = asset->assets;
    asset->asset_num = 0;
    asset->stypes = (SType*) malloc(sizeof(SType) * (tok_num + 10));
}

void ebnf_lexer(GParser *parser)
{
    int char_num = strlen(parser->input);
    int tok_num = 0;

    GToken *tokens = (GToken*) malloc(sizeof(GToken) * (char_num + 10));
    
    for (char *c = parser->input; *c; c++)
    {
        if (*c == ' ' || *c == '\n')
        {
            *c = c_null;
            continue;
        }
        else if (*c == '0' && *(c + 1) == 'x')
        {
            char hexchar[2] = {*(c + 2), *(c + 3)};
            int hexdigit[2];

            for (int i = 0; i < 2; i++)
            {
                if ('0' <= hexchar[i] && hexchar[i] <= '9')
                    hexdigit[i] = hexchar[i] - '0';
                if ('A' <= hexchar[i] && hexchar[i] <= 'F')
                    hexdigit[i] = hexchar[i] - 'A' + 10;
                if ('a' <= hexchar[i] && hexchar[i] <= 'f')
                    hexdigit[i] = hexchar[i] - 'a' + 10;
            }
            char hex = (char) (16 * hexdigit[0] + hexdigit[1]);
            *c = hex;
            *(c + 1) = c_null;
            *(c + 2) = c_null;
            *(c + 3) = c_null;
            tokens[tok_num].string = c;
            tokens[tok_num].ttype = T_STRING;
            c += 3;
        }
        else if (is_char(*c) || *c == '_')
        {
            tokens[tok_num].string = c;
            tokens[tok_num].ttype = T_IDENTITY;
            while (is_char(*(c + 1)) || is_digit(*(c + 1)) || *(c + 1) == '_') c++;
        }
        else if (*c == '\"')
        {
            *c = c_null;
            tokens[tok_num].string = c + 1;
            tokens[tok_num].ttype = T_STRING;
            c += 2;
            while (*c != '\"') c++;
            *c = c_null;
        }
        else 
        {
            switch (*c)
            {
                case C_LPAREN:
                    tokens[tok_num].string = STR_LPAREN;
                    break;
                case C_RPAREN:
                    tokens[tok_num].string = STR_RPAREN;
                    break;
                case C_LBRACE:
                    tokens[tok_num].string = STR_LBRACE;
                    break;
                case C_RBRACE:
                    tokens[tok_num].string = STR_RBRACE;
                    break;
                case C_LBRAKET:
                    tokens[tok_num].string = STR_LBRAKET;
                    break;
                case C_RBRAKET:
                    tokens[tok_num].string = STR_RBRAKET;
                    break;
                case C_DEFINE:
                    tokens[tok_num].string = STR_DEFINE;
                    break;
                case C_ALTER:
                    tokens[tok_num].string = STR_ALTER;
                    break;
                case C_CONCAT:
                    tokens[tok_num].string = STR_CONCAT;
                    break;
                case C_CRANGE:
                    tokens[tok_num].string = STR_CRANGE;
                    break;
                case C_END:
                    tokens[tok_num].string = STR_END;
                    break;
            }
            *c = c_null;
            tokens[tok_num].ttype = T_OPERATOR;
        }
        tok_num++;
    }

    parser->char_num = char_num;
    parser->tok_num = tok_num;
    parser->tokens = tokens;
}

void ebnf_parser(GParser *parser)
{
    int tok_num = parser->tok_num;

    parser->exprs = (GExpr*) malloc(sizeof(GExpr) * (tok_num + 10));
    parser->defs = (GExpr*) malloc(sizeof(GExpr) * (tok_num + 10));
    parser->pos = 0;
    parser->expr_num = 0;
    parser->def_num = 0;
    parser->lexterm_num = 0;

    parse_define(parser);

    GExpr *defs = parser->defs;
    int char_num = parser->char_num, def_num = parser->def_num;

    init_asset(parser->char_num, tok_num + 10, parser->asset);

    for (int i = 0; i < def_num; i++)
    {
        defs[i].identity.str = add_asset(defs[i].identity.str, S_IDENTITY, parser->asset);
        SType stype = (defs[i].identity.str[0] == '_'? S_BASICS : S_GRAMMAR);
        resolve_asset(defs[i].identity.expr, stype, parser->asset);
    }

    for (int i = 0; i < def_num; i++)
        index_identity(defs[i].identity.expr, parser);

    for (int i = 0; i < def_num; i++)
    {
        if (defs[i].identity.str[0] == '_')
        {
            unroll_identity(defs[i].identity.expr, parser);
            flatten_expr(defs[i].identity.expr, parser);
        }
    }

    print_parser(parser);
    print_asset(parser->asset);
}
