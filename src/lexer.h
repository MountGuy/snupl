#include "common.h"
#include "struct.h"
#include "dump.h"
#include "arena.h"
#include "bitop.h"

#define C_EPS ('\0')
#define I_EPS 0

int alloc_NFA_state(NFABuilder *builder);
void add_char(char lb, char ub, Chunk *lubs);
int find_char(char lb, char ub, Chunk *lubs);

int count_state(MetaExpr *expr, Grammar *grammar);
void gather_char(MetaExpr *expr, Chunk *lubs);

int can_trans(int start, int cdx, int end, NFABuilder *builder);
void add_trans(int start, int cdx, int end, NFABuilder *builder);

int _build_NFA(MetaExpr *expr, int start, NFABuilder *builder);
void postproc_trans(NFABuilder *builder);
NFA build_NFA(Grammar *grammar);

void init_scanner(NFA *nfa, NFAScanner *scanner);
int step_NFA(char letter, NFA *nfa, NFAScanner *scanner);
void skip_nontoken(Lexer *lexer);
Chunk lexing(char *input, NFA *nfa, Arena *arena);
