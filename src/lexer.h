#include "common.h"
#include "struct.h"
#include "character.h"

int alloc_NFA_state(NFABuilder *builder);
int find_char(char lb, char ub, NFABuilder *builder);
void _init_NFA_builder(MetaExpr *expr, NFABuilder *builder);
int init_NFA_builder(Grammar *grammar);
int _build_NFA(MetaExpr *expr, int start, NFABuilder *builder);
void build_NFA(Grammar *grammar);
