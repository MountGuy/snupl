#ifndef PARSER_H
#define PARSER_H 1

#include "common.h"
#include "bitop.h"
#include "chunk.h"

typedef struct {
    ulli *sets;
    int set_num, set_size, offset, *can_eps;
    Chunk sup_sets, sub_sets;
} SetEquBuilder;

typedef struct {
    ulli *sets;
    int *sup_sets, *sub_sets;
    int equ_num, set_num, set_size, offset, exact_set_size;
} SetEqu;

typedef struct {
    Token *tokens;
    int token_num;

    int cursor;
} Parser;

void index_node(MetaExpr *expr, int *counter);
int _null_analysis(MetaExpr *expr, int *can_eps);
int *null_analysis(Grammar *grammar, int set_num);
void regist_equ(int sub_idx, int sup_idx, SetEquBuilder *builder);
void _build_equ(MetaExpr *expr, Grammar *grammar, SetEquBuilder *builder);
SetEqu build_equ(Grammar *grammar);
int apply_equ(SetEqu *equ);
void solve_firstfollow(Grammar *grammar);

#endif
