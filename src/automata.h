#include "common.h"


void resolve_parser(Parser *parser);
Expr *resolve_expr(Expr *expr, Parser *parser);
Expr *unroll_expr(Expr *expr, Parser *parser);
