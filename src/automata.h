#include "common.h"

void add_resource(char *string, SType stype, CodeParser *c_parser);
void gather_strings(Parser *parser, CodeParser *c_parser);
void _gather_strings(Expr *expr, SType stype, CodeParser *c_parser);
