/* BISON yacc style parser generator input file
 * Now with C++!
 */

/* enable C++ mode and set minimum version */
%require "3.2"
%language "c++"
/* configure bison settings */
%define api.parser.class {Parser}
%define api.namespace {yy}
%define api.value.type variant /* much better than C unions */
%define parse.error verbose
%locations /* enable location tracking */

/* setup parameter used to pass the lexer instance around */
%parse-param {yy::Lexer& lexer}
%lex-param {yy::Lexer& lexer}

%code requires {
#include <memory>

namespace yy {
    class Lexer;
}

} /* %code requires */

/* Set boilerplate that goes into parser implementation files */
%code top {
#include "frontend/lex.h"
#include "frontend/c_ast_fwd.h"

using AstNodePtr = std::unique_ptr<AstNodePtr>;

static int yylex(yy::Parser::semantic_type *yylval,
                 yy::Parser::location_type *yylloc,
                 yy::Lexer &lexer) {
    return lexer.lex(yylval, yylloc);
}

} /* %code top */

%token IDENTIFIER STRING_LITERAL CONSTANT
%token OP_ADD OP_SUB OP_MUL OP_DIV
%token OP_GT OP_GTE OP_LT OP_LTE OP_EQ OP_NEQ
%token OP_AND OP_OR OP_NOT
%token BIT_AND BIT_OR BIT_XOR BIT_NOT
%token OP_SHL OP_SHR
%token ASSIGN ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN
%token BIT_AND_ASSIGN BIT_OR_ASSIGN BIT_NOT_ASSIGN
%token SHL_ASSIGN SHR_ASSIGN

%token ARROW DOT L_BRACE R_BRACE L_BRACKET R_BRACKET
%token BOOL SHORT INT LONG UNSIGNED SIGNED FLOAT DOUBLE CHAR VOID
%token DO WHILE FOR IF ELSE
%token STRUCT UNION ENUM

%type<AstNodePtr> primary_expression statement top_level_declaration declaration function_declaration

%%
translation_unit
    : top_level_declaration
    | translation_unit top_level_declaration
    ;

top_level_declaration
    : function_declaration
    | declaration
    ;

function_declaration
    : type IDENTIFIER '(' function_argument_declarations ')' compound_statement
    | type IDENTIFIER '(' ')' compound_statement
    ;

primary_expression
    : IDENTIFIER
    | STRING_LITERAL
    | CONSTANT
    ;
 
%%
 
