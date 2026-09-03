#include "common.h"


void resolve_parser(Parser *parser);
void resolve_expr(Expr **expr, Parser *parser);
void unroll_expr(Expr *expr, Parser *parser);
