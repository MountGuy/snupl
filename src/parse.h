#include "struct.h"
#include "common.h"
#include "chunk.h"
#include "bitop.h"
#include "dump.h"

void index_node(MetaExpr *expr, int *counter);
int _null_analysis(MetaExpr *expr, int *can_eps);
int *null_analysis(Grammar *grammar, int set_num);
void regist_equ(int sub_idx, int sup_idx, SetEquBuilder *builder);
void _build_equ(MetaExpr *expr, Grammar *grammar, SetEquBuilder *builder);
SetEqu build_equ(Grammar *grammar);
int apply_equ(SetEqu *equ);
void solve_firstfollow(Grammar *grammar);
