#include "common.h"

void gather_strings(GParser *parser, CParser *c_parser);
void add_resource(char *string, SType stype, CParser *c_parser);
void sort_resource(CParser *parser);
void _gather_strings(GExpr *expr, SType stype, CParser *c_parser);