#ifndef MIPS_LEXER_HPP
#define MIPS_LEXER_HPP

#include "token.hpp"
#include "ctl/array.hpp"
#include "ctl/maybe.hpp"

using namespace ctl;

struct Lexer {
    Lexer(StringView source, Allocator& allocator);

    Token next();

    Maybe<Array<Token>> tokenize();

    Bool       has_error()      const { return error_; }
    StringView error_message()  const { return error_msg_; }
    Location   error_location() const { return error_loc_; }

private:
    char     peek_byte(Ulen offset = 0) const;
    Bool     at_end() const { return pos_ >= source_.length(); }
    void     advance();
    Location current_loc() const;

    Token make_token(TokenKind kind, Ulen start_pos, Location start_loc) const;
    Token make_invalid(StringView msg, Ulen start_pos, Location start_loc);

    void skip_horizontal_whitespace();
    Bool skip_comment();

    Token scan_register(Ulen start, Location start_loc);
    Token scan_directive(Ulen start, Location start_loc);
    Token scan_identifier(Ulen start, Location start_loc);
    Token scan_number(Ulen start, Location start_loc);
    Token scan_string(Ulen start, Location start_loc);
    Token scan_char(Ulen start, Location start_loc);

    StringView source_;
    Allocator& allocator_;
    Ulen       pos_       = 0;
    Uint32     line_      = 1;
    Uint32     column_    = 1;
    Bool       error_     = false;
    StringView error_msg_;
    Location   error_loc_;
};

#endif // MIPS_LEXER_HPP
