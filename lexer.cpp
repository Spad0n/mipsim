#include "lexer.hpp"

static CTL_FORCEINLINE Bool is_id_start(char c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '_';
}

static CTL_FORCEINLINE Bool is_id_cont(char c) {
    return is_id_start(c) || (c >= '0' && c <= '9');
}

static CTL_FORCEINLINE Bool is_dec_digit(char c) {
    return c >= '0' && c <= '9';
}

static CTL_FORCEINLINE Bool is_hex_digit(char c) {
    return is_dec_digit(c)
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

static Uint32 hex_value(char c) {
    if (c >= '0' && c <= '9') return Uint32(c - '0');
    if (c >= 'a' && c <= 'f') return Uint32(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return Uint32(c - 'A' + 10);
    return 0xFFFFFFFFu;
}

Lexer::Lexer(StringView source, Allocator& allocator)
    : source_{source}
    , allocator_{allocator}
{}

char Lexer::peek_byte(Ulen offset) const {
    const auto p = pos_ + offset;
    if (p >= source_.length()) return '\0';
    return source_[p];
}

Location Lexer::current_loc() const {
    return Location{ line_, column_, static_cast<Uint32>(pos_) };
}

// Advance by one rune, updating line/column. Used for both ASCII and
// UTF-8. For ASCII (< 0x80) this is a fast 1-byte path.
void Lexer::advance() {
    if (at_end()) return;
    const auto c = static_cast<unsigned char>(source_[pos_]);
    if (c == '\n') {
        line_++;
        column_ = 1;
        pos_++;
        return;
    }
    if (c < 0x80) {
        column_++;
        pos_++;
        return;
    }
    // UTF-8 multi-byte: decode to advance by the full rune length.
    auto rest = source_.slice(pos_).cast<const Uint8>();
    Rune r{0};
    auto n = Rune::decode_utf8(rest, r);
    if (n == 0) n = 1; // invalid UTF-8: skip 1 byte to make progress
    column_++;
    pos_ += n;
}

Token Lexer::make_token(TokenKind kind, Ulen start_pos, Location start_loc) const {
    Token t;
    t.kind = kind;
    t.loc  = start_loc;
    t.text = source_.subrange(start_pos, pos_);
    return t;
}

Token Lexer::make_invalid(StringView msg, Ulen start_pos, Location start_loc) {
    if (!error_) {
        error_     = true;
        error_msg_ = msg;
        error_loc_ = start_loc;
    }
    return make_token(TokenKind::INVALID, start_pos, start_loc);
}

void Lexer::skip_horizontal_whitespace() {
    while (!at_end()) {
        const auto c = peek_byte();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

Bool Lexer::skip_comment() {
    const auto c = peek_byte();
    if (c != '#' && c != ';') return false;
    // Consume up to (but not including) the newline. The newline itself
    // is left to be emitted as a NEWLINE token.
    while (!at_end() && peek_byte() != '\n') {
        advance();
    }
    return true;
}

Token Lexer::next() {
    for (;;) {
        skip_horizontal_whitespace();
        if (skip_comment()) continue;
        break;
    }

    const auto start_loc = current_loc();
    const auto start     = pos_;

    if (at_end()) {
        return make_token(TokenKind::ENDOF, start, start_loc);
    }

    const auto c = peek_byte();

    if (c == '\n') {
        advance();
        return make_token(TokenKind::NEWLINE, start, start_loc);
    }
    if (c == '$')  return scan_register  (start, start_loc);
    if (c == '.')  return scan_directive (start, start_loc);
    if (is_id_start(c)) return scan_identifier(start, start_loc);
    if (is_dec_digit(c)) return scan_number(start, start_loc);
    if (c == '"')  return scan_string    (start, start_loc);
    if (c == '\'') return scan_char      (start, start_loc);

    // Single-character punctuation.
    advance();
    switch (c) {
    case ',': return make_token(TokenKind::COMMA,  start, start_loc);
    case '(': return make_token(TokenKind::LPAREN, start, start_loc);
    case ')': return make_token(TokenKind::RPAREN, start, start_loc);
    case ':': return make_token(TokenKind::COLON,  start, start_loc);
    case '+': return make_token(TokenKind::PLUS,   start, start_loc);
    case '-': return make_token(TokenKind::MINUS,  start, start_loc);
    }

    return make_invalid(StringView("unexpected character"), start, start_loc);
}

Token Lexer::scan_register(Ulen start, Location start_loc) {
    advance(); // skip '$'

    const auto name_start = pos_;
    while (!at_end() && is_id_cont(peek_byte())) {
        advance();
    }
    const auto name = source_.subrange(name_start, pos_);

    if (name.is_empty()) {
        return make_invalid(StringView("expected register name after '$'"),
                            start, start_loc);
    }

    Uint32 num = 0;
    if (!lookup_register(name, num)) {
        return make_invalid(StringView("unknown register name"), start, start_loc);
    }

    Token t = make_token(TokenKind::REGISTER, start, start_loc);
    t.int_value = num;
    return t;
}

Token Lexer::scan_directive(Ulen start, Location start_loc) {
    advance(); // skip '.'

    while (!at_end() && is_id_cont(peek_byte())) {
        advance();
    }
    const auto name = source_.subrange(start, pos_); // includes leading '.'

    DirectiveKind k;
    if (lookup_directive(name, k)) {
        Token t = make_token(TokenKind::DIRECTIVE, start, start_loc);
        t.sub = static_cast<Uint32>(k);
        return t;
    }
    return make_invalid(StringView("unknown directive"), start, start_loc);
}

Token Lexer::scan_identifier(Ulen start, Location start_loc) {
    advance();
    while (!at_end() && is_id_cont(peek_byte())) {
        advance();
    }
    const auto name = source_.subrange(start, pos_);

    InstrKind ik;
    if (lookup_instr(name, ik)) {
        Token t = make_token(TokenKind::INSTR, start, start_loc);
        t.sub = static_cast<Uint32>(ik);
        return t;
    }
    PseudoKind pk;
    if (lookup_pseudo(name, pk)) {
        Token t = make_token(TokenKind::PSEUDO, start, start_loc);
        t.sub = static_cast<Uint32>(pk);
        return t;
    }
    return make_token(TokenKind::IDENT, start, start_loc);
}

Token Lexer::scan_number(Ulen start, Location start_loc) {
    Uint64 value = 0;
    Uint32 base  = 10;

    const auto c0 = peek_byte();
    const auto c1 = peek_byte(1);

    if (c0 == '0' && (c1 == 'x' || c1 == 'X')) {
        advance(); advance();
        base = 16;
    } else if (c0 == '0' && (c1 == 'b' || c1 == 'B')) {
        advance(); advance();
        base = 2;
    } else if (c0 == '0' && (c1 == 'o' || c1 == 'O')) {
        advance(); advance();
        base = 8;
    }

    Bool any = false;
    while (!at_end()) {
        const auto c = peek_byte();
        if (c == '_') { advance(); continue; }

        Uint32 d;
        if (base == 16 && is_hex_digit(c)) {
            d = hex_value(c);
        } else if (is_dec_digit(c)) {
            d = Uint32(c - '0');
        } else {
            break;
        }
        if (d >= base) {
            return make_invalid(StringView("digit out of range for base"),
                                start, start_loc);
        }
        value = value * base + d;
        advance();
        any = true;
    }

    if (!any) {
        return make_invalid(StringView("expected digits in numeric literal"),
                            start, start_loc);
    }

    Token t = make_token(TokenKind::INTEGER, start, start_loc);
    t.int_value = value;
    return t;
}

Token Lexer::scan_string(Ulen start, Location start_loc) {
    advance(); // opening '"'
    const auto content_start = pos_;
    while (!at_end()) {
        const auto c = peek_byte();
        if (c == '"')  break;
        if (c == '\n') {
            return make_invalid(StringView("unterminated string"),
                                start, start_loc);
        }
        if (c == '\\') {
            advance();
            if (at_end() || peek_byte() == '\n') {
                return make_invalid(StringView("unterminated string"),
                                    start, start_loc);
            }
            advance();
            continue;
        }
        advance();
    }
    if (at_end()) {
        return make_invalid(StringView("unterminated string"), start, start_loc);
    }
    const auto content_end = pos_;
    advance(); // closing '"'

    Token t = make_token(TokenKind::STRING, start, start_loc);
    t.text  = source_.subrange(content_start, content_end);
    return t;
}

Token Lexer::scan_char(Ulen start, Location start_loc) {
    advance(); // opening '\''
    if (at_end() || peek_byte() == '\n') {
        return make_invalid(StringView("unterminated char literal"),
                            start, start_loc);
    }

    Uint32 value = 0;

    if (peek_byte() == '\\') {
        advance();
        if (at_end()) {
            return make_invalid(StringView("unterminated char escape"),
                                start, start_loc);
        }
        const auto e = peek_byte();
        advance();
        switch (e) {
        case 'n':  value = '\n'; break;
        case 'r':  value = '\r'; break;
        case 't':  value = '\t'; break;
        case '0':  value = 0;    break;
        case '\\': value = '\\'; break;
        case '\'': value = '\''; break;
        case '"':  value = '"';  break;
        case 'x': {
            const auto h0 = peek_byte();
            const auto h1 = peek_byte(1);
            if (!is_hex_digit(h0) || !is_hex_digit(h1)) {
                return make_invalid(StringView("invalid \\x escape"),
                                    start, start_loc);
            }
            value = (hex_value(h0) << 4) | hex_value(h1);
            advance(); advance();
            break;
        }
        default:
            return make_invalid(StringView("unknown escape sequence"),
                                start, start_loc);
        }
    } else {
        // Decode a single UTF-8 codepoint.
        auto rest = source_.slice(pos_).cast<const Uint8>();
        Rune r{0};
        const auto n = Rune::decode_utf8(rest, r);
        if (n == 0) {
            return make_invalid(StringView("invalid UTF-8 in char literal"),
                                start, start_loc);
        }
        value = static_cast<Uint32>(r);
        for (Ulen i = 0; i < n; i++) advance();
    }

    if (peek_byte() != '\'') {
        return make_invalid(StringView("expected closing '\\''"),
                            start, start_loc);
    }
    advance();

    Token t = make_token(TokenKind::CHAR, start, start_loc);
    t.int_value = value;
    return t;
}

Maybe<Array<Token>> Lexer::tokenize() {
    Array<Token> result{allocator_};
    for (;;) {
        const auto t = next();
        if (!result.push_back(t)) {
            return {};
        }
        if (t.kind == TokenKind::ENDOF) break;
    }
    return result;
}
