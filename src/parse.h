#include "struct.h"
#include "common.h"
#include "chunk.h"
#include "bitop.h"
#include "dump.h"

int count_set(MetaExpr *expr, Grammar *grammar);
void _build_equ(MetaExpr *expr, Grammar *grammar, SetEquBuilder *builder);
SetEqu build_equ(Grammar *grammar);
void solve_firstfollow(Grammar *grammar);
int count_one2(Set *first, Set *follow, int set_num);
