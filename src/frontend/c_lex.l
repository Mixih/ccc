/* FLEX lexer input file
 * c++ flavored config for these old tool courtesy:
 * https://learnmoderncpp.com/2020/12/18/generating-c-programs-with-flex-and-bison-3/
 * and https://github.com/ezaquarii/bison-flex-cpp-example
 */

%option yylineno
%option noyywrap
%option c++
%option yyclass="Lexer"

%{
#include <string>
#include "parse.tab.hpp"
#include "frontend/lex.h"

using TokType = yy::Parser::token_type;
using namespace yy;

#define YY_USER_ACTION update_location(yylloc, yytext);
%}

%%

[[:space:]]+             /* skip any whitespace */

%%

// Credit where credit is due:
// https://stackoverflow.com/questions/656703/how-does-flex-support-bison-location-exactly
void Lexer::update_location(Parser::location_type *yylloc,
                            const char *yytext) {
    yylloc->begin.line = yylloc->end.line;
    yylloc->begin.column = yylloc->end.column;
    yylloc->begin.filename = &filename;
    yylloc->end.filename = &filename;
    for (int i = 0; yytext[i] != '\0'; i++) {
        if (yytext[i] == '\n') {
            yylloc->end.line++;
            yylloc->end.column = 1;
        } else {
            yylloc->end.column++;
        }
    }
}
