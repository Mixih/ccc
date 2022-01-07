#ifndef LEX_H_
#define LEX_H_

#include "parse.tab.hpp"
// prevent redefining headers
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

// override the lexer declaration for bison interop
#undef YY_DECL
#define YY_DECL                                                                          \
    int yy::Lexer::lex(yy::Parser::semantic_type* yylval,                                \
                       yy::Parser::location_type* yylloc)

namespace yy {

class Lexer : public yyFlexLexer {
private:
    std::string filename;

    /**
     * Update bison's location tracker to match flex's state.
     *
     * WARNING: This will set the yylloc's *filename to reference the lexer's
     * internal state. Always copy when extracting! (this is a code smell, but
     * yylex and yyparse are pretty much married already anyways...)
     */
    void update_location(Parser::location_type* yylloc, const char* yytext);
    int create_token(Parser::semantic_type* yylval, Parser::location_type* yylloc,
                     int tag, const char* lexeme);
    void fail(Parser::location_type* yylloc, const char* message);

public:
    Lexer(const std::string file, std::istream& arg_yyin, std::ostream& arg_yyout)
        : yyFlexLexer(arg_yyin, arg_yyout), filename(file) {}

    int lex(Parser::semantic_type* yylval, Parser::location_type* yylloc);
};

} // namespace yy

#endif /* LEX_H_ */
