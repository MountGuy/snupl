#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "febnf.h"

void advance_fparser(fParser *parser)
{
    parser->pos++;
}

Token *peek_tok_f(fParser *parser)
{
    return parser->tokens + parser->pos;
}

fExpr *alloc_fexpr(fParser *parser)
{
    fExpr *expr = parser->exprs + parser->expr_num;
    parser->expr_num++;

    return expr;
}

int febnf_parser(Lexer *lexer, Token *tokens)
{
    fParser parser = {
        .tokens = tokens,
        .pos = 0,
        .tok_num = lexer->tok_num,
        .exprs = (fExpr*) malloc(sizeof(fExpr) * (lexer->tok_num + 10)),
        .expr_num = 0,
    };

    while (parser.pos < parser.tok_num)
        fparse_define(&parser);
    return 0;
}

fExpr *fparse_define(fParser *parser)
{
    Token *tok1 = peek_tok_f(parser); advance_fparser(parser);
    Token *tok2 = peek_tok_f(parser); advance_fparser(parser);

    if (!(tok1->ttype == T_IDENTITY && tok2->ttype == T_OPERATOR && tok2->string == S_DEFINE))
    {
        printf("definition format error\n");
        exit(1);
    }

    fExpr *curr_expr = fparse_alter(parser);
    Token *tok3 = peek_tok_f(parser);
    if (tok3->string != S_END)
    {
        printf("definition format error\n");
        exit(1);
    }
    advance_fparser(parser);
    fExpr *new_expr = alloc_fexpr(parser);
    new_expr->kind = E_DEF;
    new_expr->definition.string = tok1->string;
    new_expr->definition.expr = curr_expr;

    print_fexpr(new_expr);
    return new_expr;
}


fExpr *fparse_alter(fParser *parser)
{
    int expr_num = 0, last_tok = parser->tok_num - parser->pos + 10;
    fExpr **exprs = (fExpr**) malloc(sizeof(exprs) * last_tok);

    while (true)
    {
        exprs[expr_num] = fparse_concat(parser);
        char tok_char = peek_tok_f(parser)->string[0];
        expr_num++;
        
        if (tok_char == C_END ||
            tok_char == C_RPAREN || tok_char == C_RBRACE || tok_char == C_RBRAKET)
            break;
        if (tok_char == C_ALTER)
        {
            advance_fparser(parser);
            continue;
        }
        printf("error\n");
        exit(1);
    }
    if (expr_num == 1)
    {
        fExpr *expr = exprs[0];
        free(exprs);
        return expr;
    }
    else
    {
        fExpr *expr = alloc_fexpr(parser);
        expr->kind = E_ALTER;
        expr->nary.exprs = (fExpr**) malloc(sizeof(fExpr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, exprs, sizeof(fExpr*) * expr_num);
        free(exprs);
        return expr;
    }
}


fExpr *fparse_concat(fParser *parser)
{
    int expr_num = 0, last_tok = parser->tok_num - parser->pos + 10;
    fExpr **exprs = (fExpr**) malloc(sizeof(exprs) * last_tok);

    while (true)
    {
        exprs[expr_num] = fparse_primary(parser);
        char tok_char = peek_tok_f(parser)->string[0];
        expr_num++;

        if (tok_char == C_ALTER || tok_char == C_END ||
            tok_char == C_RPAREN || tok_char == C_RBRACE || tok_char == C_RBRAKET)
            break;
        if (tok_char == C_CONCAT)
        {
            advance_fparser(parser);
            continue;
        }
        printf("error\n");
    }
    if (expr_num == 1)
    {
        fExpr *expr = exprs[0];
        free(exprs);
        return expr;
    }
    else
    {
        fExpr *expr = alloc_fexpr(parser);
        expr->kind = E_CONCAT;
        expr->nary.exprs = (fExpr**) malloc(sizeof(fExpr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, exprs, sizeof(fExpr*) * expr_num);
        free(exprs);
        return expr;
    }
}

fExpr *fparse_primary(fParser *parser)
{
    Token *curr_token = peek_tok_f(parser);
    TType ttype = curr_token->ttype;
    char *string = curr_token->string;

    switch (ttype)
    {
        case T_STRING:
        case T_IDENTITY:
        {
            advance_fparser(parser);
            fExpr *expr = alloc_fexpr(parser);
            expr->kind = (ttype == T_STRING? E_LETS: E_IDENT);
            expr->identity.string = string;
            return expr;
        }
        case T_OPERATOR:
        {
            if (string == S_LPAREN || string == S_LBRACE || string == S_LBRAKET)
            {
                advance_fparser(parser);
                fExpr *curr_expr = fparse_alter(parser);
                Token *next_token = peek_tok_f(parser);
                char *next_string = next_token->string;

                if (string == S_LPAREN && next_string == S_RPAREN)
                {
                    advance_fparser(parser);

                    return curr_expr;
                }
                else if ((string == S_LBRACE && next_string == S_RBRACE)
                      || (string == S_LBRAKET && next_string == S_RBRAKET) )
                {
                    advance_fparser(parser);
                    fExpr *new_expr = alloc_fexpr(parser);
                    new_expr->kind = (string == S_LBRACE? E_REP: E_OPT);
                    new_expr->nary.expr_num = 1;
                    new_expr->nary.exprs = (fExpr**) malloc(sizeof(fExpr*));
                    new_expr->nary.exprs[0] = curr_expr;

                    return new_expr;                    
                }
            }

            printf("1. unexpected parsing: parse primary, %s %s\n", string, peek_tok_f(parser)->string);
            exit(1);
        }
        default:
        {
            printf("2. unexpected parsing: parse primary\n");
            exit(1);
        }
    }
}