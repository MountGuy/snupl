#include "common.h"

void add_resource(char *string, SType stype, CParser *c_parser);
void gather_strings(GParser *parser, CParser *c_parser);
void _gather_strings(GExpr *expr, SType stype, CParser *c_parser);
