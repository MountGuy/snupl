#ifndef PARSER_H
#define PARSER_H 1

#include "common.h"
#include "chunk.h"

typedef struct {
    ulli *sets;
    int set_num, set_size, offset, exact_set_size;
} FirstFollow;

typedef struct {
    FirstFollow *ff;
    int *can_eps;
    Chunk sub_sets, sup_sets;
} SetEqu;

typedef struct {
    Token *tokens;
    int token_num;

    int cursor;
} Parser;

void index_node(MetaExpr *expr, int *counter);
int _null_analysis(MetaExpr *expr, int *can_eps);
void null_analysis(Grammar *grammar, int *can_eps, int set_num);
void regist_equ(int sub_idx, int sup_idx, SetEqu *set_equ);
void build_equ(MetaExpr *expr, Grammar *grammar, SetEqu *set_equ);
int solve_equ(SetEqu *equ);
FirstFollow solve_ff(Grammar *grammar);

#endif
