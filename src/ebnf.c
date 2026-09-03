#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"
#include "analysis.h"
#include "automata.h"


void ebnf_lexer(Parser *parser)
{
    int char_num = strlen(parser->input);
    int tok_num = 0;

    parser->char_num = char_num;
    parser->assets = (char*) malloc(sizeof(char) * (char_num + 10));
    parser->starts = (char**) malloc(sizeof(char*) * (char_num + 10));
    parser->top = parser->assets;
    parser->asset_num = 0;
    parser->asset_types = (TType*) malloc(sizeof(TType) * (char_num + 10));

    Token *tokens = (Token*) malloc(sizeof(Token) * (char_num + 10));
    
    for (char *c = parser->input; *c; c++)
    {
        if (*c == ' ' || *c == '\n')
        {
            *c = c_null;
            continue;
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
                    tokens[tok_num].string = S_LPAREN;
                    break;
                case C_RPAREN:
                    tokens[tok_num].string = S_RPAREN;
                    break;
                case C_LBRACE:
                    tokens[tok_num].string = S_LBRACE;
                    break;
                case C_RBRACE:
                    tokens[tok_num].string = S_RBRACE;
                    break;
                case C_LBRAKET:
                    tokens[tok_num].string = S_LBRAKET;
                    break;
                case C_RBRAKET:
                    tokens[tok_num].string = S_RBRAKET;
                    break;
                case C_DEFINE:
                    tokens[tok_num].string = S_DEFINE;
                    break;
                case C_ALTER:
                    tokens[tok_num].string = S_ALTER;
                    break;
                case C_CONCAT:
                    tokens[tok_num].string = S_CONCAT;
                    break;
                case C_END:
                    tokens[tok_num].string = S_END;
                    break;
            }
            *c = c_null;
            tokens[tok_num].ttype = T_OPERATOR;
        }
        tok_num++;
    }

    for (int i = 0; i < tok_num; i++)
    {
        if (tokens[i].ttype == T_IDENTITY || tokens[i].ttype == T_STRING)
            tokens[i].string = search_asset(tokens[i].string, tokens[i].ttype, parser);
    }

    parser->tok_num = tok_num;
    parser->tokens = tokens;
}

void ebnf_parser(Parser *parser)
{
    int tok_num = parser->tok_num;

    parser->exprs = (Expr*) malloc(sizeof(Expr) * (tok_num + 10));
    parser->defs = (Expr*) malloc(sizeof(Expr) * (tok_num + 10));
    parser->pos = 0;
    parser->expr_num = 0;
    parser->def_num = 0;

    parse_define(parser);
    for (int i = 0; i < parser->def_num; i++)
        resolve_refer(parser->defs[i].identity.expr, parser);
    
    resolve_parser(parser);

    // null_test(parser);
}

void resolve_refer(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                resolve_refer(expr->nary.exprs[i], parser);
            break;
        case E_IDENTITY:
            for (int i = 0; i < parser->def_num; i++)
                if (parser->defs[i].identity.str == expr->identity.str)
                {
                    *expr = parser->defs[i];
                    expr->kind = E_IDENTITY;
                    break;
                }
            break;
        case E_STRING:
            break;
        case E_DEFINE:
        default:
            printf("error while resolving\n");
            exit(1);
    }
}

Expr *parse_define(Parser *parser)
{
    int def_num = 0;

    while (parser->pos < parser->tok_num)
    {
        Token *tok_id = advance_parser(parser);
        Token *tok_def = advance_parser(parser);

        if (!(
            tok_id->ttype == T_IDENTITY &&
            tok_def->ttype == T_OPERATOR &&
            tok_def->string == S_DEFINE
        ))
        {
            printf("definition format error\n");
            exit(1);
        }

        Expr *expr = parse_alter(parser);
        Token *tok_end = advance_parser(parser);

        if (!(
            tok_end->ttype == T_OPERATOR &&
            tok_end->string == S_END
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
    }

    parser->def_num = def_num;

    return parser->defs;
}

Expr *parse_alter(Parser *parser)
{
    int expr_num = 0;
    Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->tok_num);

    while (B_TRUE)
    {
        buffer[expr_num] = parse_concat(parser);
        Token *token = peek_tok(parser);
        char *string = peek_tok(parser)->string;
        TType ttype = token->ttype;
        expr_num++;
        
        if ((
            string == S_END ||
            string == S_RPAREN ||
            string == S_RBRACE ||
            string == S_RBRAKET) &&
            ttype == T_OPERATOR
        )
            break;
        else if (ttype == T_OPERATOR && string == S_ALTER)
        {
            advance_parser(parser);
            continue;
        }
        else
        {
            printf("error\n");
            exit(1);
        }
    }
    if (expr_num == 1)
    {
        Expr *expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        Expr *expr = alloc_expr(parser);
        expr->kind = E_ALTER;
        expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, buffer, sizeof(Expr*) * expr_num);
        free(buffer);
        
        return expr;
    }
}

Expr *parse_concat(Parser *parser)
{
    int expr_num = 0;
    Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->tok_num);

    while (B_TRUE)
    {
        buffer[expr_num] = parse_primary(parser);
        Token *token = peek_tok(parser);
        char *string = peek_tok(parser)->string;
        TType ttype = token->ttype;
        expr_num++;

        if ((
            string == S_ALTER ||
            string == S_END ||
            string == S_RPAREN ||
            string == S_RBRACE ||
            string == S_RBRAKET)&&
            ttype == T_OPERATOR
        )
            break;
        else if (ttype == T_OPERATOR && string == S_CONCAT)
        {
            advance_parser(parser);
            continue;
        }
        else
        {
            printf("error\n");
            exit(1);
        }
    }
    if (expr_num == 1)
    {
        Expr *expr = buffer[0];
        free(buffer);
        return expr;
    }
    else
    {
        Expr *expr = alloc_expr(parser);
        expr->kind = E_CONCAT;
        expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, buffer, sizeof(Expr*) * expr_num);
        free(buffer);

        return expr;
    }
}

Expr *parse_primary(Parser *parser)
{
    Token *token = advance_parser(parser);
    TType ttype = token->ttype;
    char *string = token->string;
    Expr *expr;

    switch (ttype)
    {
        case T_STRING:
            expr = alloc_expr(parser);
            expr->kind = E_STRING;
            expr->string.str = string;
            return expr;
        case T_IDENTITY:
            expr = alloc_expr(parser);
            expr->kind = E_IDENTITY;
            expr->identity.idx = 0;
            expr->identity.str = string;
            expr->identity.expr = p_null;
            return expr;
        case T_OPERATOR:
            if (
                string == S_LPAREN ||
                string == S_LBRACE ||
                string == S_LBRAKET
            )
            {
                Expr *body_expr = parse_alter(parser);
                Token *next_token = advance_parser(parser);
                char *next_string = next_token->string;

                if (string == S_LPAREN && next_string == S_RPAREN)
                    return body_expr;
                else if (
                    (string == S_LBRACE && next_string == S_RBRACE) ||
                    (string == S_LBRAKET && next_string == S_RBRAKET)
                )
                {
                    Expr *expr = alloc_expr(parser);
                    expr->kind = (string == S_LBRACE? E_REPEAT: E_OPTION);
                    expr->nary.expr_num = 1;
                    expr->nary.exprs = (Expr**) malloc(sizeof(Expr*));
                    expr->nary.exprs[0] = body_expr;

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
