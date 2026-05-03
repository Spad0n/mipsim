#include "parser.hpp"

Parser::Parser(Slice<Token> tokens, Allocator& alloc)
    : tokens_{tokens}
    , alloc_{alloc}
    , stmts_{alloc}
    , operands_{alloc}
    , errors_{alloc}
{}

const Token& Parser::peek(Ulen offset) const {
    const auto p = pos_ + offset;
    if (p >= tokens_.length()) return tokens_[tokens_.length() - 1]; // ENDOF
    return tokens_[p];
}

Bool Parser::at_end() const {
    return peek().kind == TokenKind::ENDOF;
}

void Parser::advance() {
    if (pos_ + 1 < tokens_.length()) pos_ += 1;
}

Bool Parser::match(TokenKind k) {
    if (peek().kind != k) return false;
    advance();
    return true;
}

Bool Parser::expect(TokenKind k, StringView what) {
    if (peek().kind == k) { advance(); return true; }
    return record_error(peek().loc, what);
}

Bool Parser::record_error(Location loc, StringView msg) {
    // Allocation failure here is a hard stop, but we still return false
    // so callers know the statement is bad.
    Diagnostic d{ loc, msg };
    (void)errors_.push_back(move(d));
    return false;
}

void Parser::skip_newlines() {
    while (peek().kind == TokenKind::NEWLINE) advance();
}

void Parser::recover_to_newline() {
    while (!at_end() && peek().kind != TokenKind::NEWLINE) advance();
}

// -- Top-level loop ---------------------------------------------------

Bool Parser::parse() {
    while (!at_end()) {
        skip_newlines();
        if (at_end()) break;
        if (!parse_one()) {
            recover_to_newline();
        }
    }
    return true;
}

Bool Parser::parse_one() {
    // Greedily consume any leading labels (e.g. "loop: foo: addiu ...").
    while (peek().kind == TokenKind::IDENT && peek(1).kind == TokenKind::COLON) {
        if (!parse_label_def()) return false;
    }

    // After labels we accept either end-of-line or a body statement.
    const auto& t = peek();
    if (t.kind == TokenKind::NEWLINE || t.kind == TokenKind::ENDOF) {
        return true;
    }

    Stmt s;
    s.loc       = t.loc;
    s.op_offset = static_cast<Uint32>(operands_.length());

    switch (t.kind) {
    case TokenKind::INSTR: {
        const auto kind = t.as_instr();
        advance();
        if (!parse_instr(kind, s)) return false;
        break;
    }
    case TokenKind::PSEUDO: {
        const auto kind = t.as_pseudo();
        advance();
        if (!parse_pseudo(kind, s)) return false;
        break;
    }
    case TokenKind::DIRECTIVE: {
        const auto kind = t.as_directive();
        advance();
        if (!parse_directive(kind, s)) return false;
        break;
    }
    default:
        return record_error(t.loc, StringView("expected instruction or directive"));
    }

    if (!finalize_stmt(s, s.op_offset)) return false;

    // Statements must end at a newline or EOF.
    if (peek().kind != TokenKind::NEWLINE && peek().kind != TokenKind::ENDOF) {
        return record_error(peek().loc, StringView("unexpected token after statement"));
    }
    return true;
}

Bool Parser::parse_label_def() {
    const auto& name_tok = peek();
    const auto& colon    = peek(1);
    // Caller already verified IDENT COLON.
    Stmt s;
    s.kind = Stmt::Kind::LABEL_DEF;
    s.loc  = name_tok.loc;
    s.text = name_tok.text;
    advance(); // IDENT
    advance(); // COLON
    (void)colon;
    return stmts_.push_back(move(s));
}

Bool Parser::finalize_stmt(Stmt& s, Uint32 op_start) {
    s.op_offset = op_start;
    s.op_count  = static_cast<Uint32>(operands_.length()) - op_start;
    return stmts_.push_back(move(s));
}

// -- Operand primitives -----------------------------------------------

Bool Parser::parse_signed_integer(Sint64& out, Location& loc) {
    Bool neg = false;
    if (match(TokenKind::PLUS)) {
        // explicit positive
    } else if (match(TokenKind::MINUS)) {
        neg = true;
    }
    const auto& t = peek();
    loc = t.loc;
    if (t.kind != TokenKind::INTEGER && t.kind != TokenKind::CHAR) {
        return record_error(t.loc, StringView("expected integer literal"));
    }
    // We accept arbitrary 64-bit unsigned and let the assembler range-check.
    out = static_cast<Sint64>(t.int_value);
    if (neg) out = -out;
    advance();
    return true;
}

Bool Parser::parse_register_op() {
    const auto& t = peek();
    if (t.kind != TokenKind::REGISTER) {
        return record_error(t.loc, StringView("expected register"));
    }
    Operand op;
    op.kind = Operand::Kind::REG;
    op.reg  = static_cast<Uint32>(t.int_value);
    op.loc  = t.loc;
    advance();
    return operands_.push_back(move(op));
}

Bool Parser::parse_signed_imm_op() {
    Operand op;
    op.kind = Operand::Kind::IMM;
    op.loc  = peek().loc;
    if (!parse_signed_integer(op.imm, op.loc)) return false;
    return operands_.push_back(move(op));
}

Bool Parser::parse_imm_or_label_op() {
    const auto& t = peek();
    if (t.kind == TokenKind::IDENT) {
        Operand op;
        op.kind  = Operand::Kind::LABEL;
        op.label = t.text;
        op.loc   = t.loc;
        advance();
        return operands_.push_back(move(op));
    }
    return parse_signed_imm_op();
}

Bool Parser::parse_label_op() {
    const auto& t = peek();
    if (t.kind != TokenKind::IDENT) {
        return record_error(t.loc, StringView("expected label"));
    }
    Operand op;
    op.kind  = Operand::Kind::LABEL;
    op.label = t.text;
    op.loc   = t.loc;
    advance();
    return operands_.push_back(move(op));
}

Bool Parser::parse_memory_op() {
    // Two accepted forms:
    //   imm "(" register ")"
    //        "(" register ")"   -- implicit zero offset
    Operand op;
    op.kind = Operand::Kind::MEM;
    op.loc  = peek().loc;

    Sint64 imm = 0;
    if (peek().kind != TokenKind::LPAREN) {
        Location iloc;
        if (!parse_signed_integer(imm, iloc)) return false;
    }
    op.imm = imm;

    if (!expect(TokenKind::LPAREN, StringView("expected '('"))) return false;
    const auto& reg = peek();
    if (reg.kind != TokenKind::REGISTER) {
        return record_error(reg.loc, StringView("expected register inside '()'"));
    }
    op.reg = static_cast<Uint32>(reg.int_value);
    advance();
    if (!expect(TokenKind::RPAREN, StringView("expected ')'"))) return false;

    return operands_.push_back(move(op));
}

static CTL_FORCEINLINE Bool comma_ok(Parser& p, Bool& ok) {
    // Helper used inline to chain operand parses with a comma between them.
    // Returns (via ok) whether to keep going.
    (void)p; (void)ok;
    return true;
}

// -- Instruction operands (driven by InstrFormat) ---------------------

Bool Parser::parse_instr_operands(InstrFormat fmt) {
    auto comma = [&]() { return expect(TokenKind::COMMA, StringView("expected ','")); };

    switch (fmt) {
    case InstrFormat::R_NONE:
        return true;

    case InstrFormat::R_RS:
        return parse_register_op();

    case InstrFormat::R_RD:
        return parse_register_op();

    case InstrFormat::R_RD_RS:
        return parse_register_op() && comma() && parse_register_op();

    case InstrFormat::R_RS_RT:
        return parse_register_op() && comma() && parse_register_op();

    case InstrFormat::R_RD_RS_RT:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_register_op();

    case InstrFormat::R_RD_RT_SA:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_signed_imm_op();

    case InstrFormat::R_RD_RT_RS:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_register_op();

    case InstrFormat::I_RT_RS_IMM:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_signed_imm_op();

    case InstrFormat::I_RT_IMM:
        return parse_register_op() && comma() && parse_signed_imm_op();

    case InstrFormat::I_RS_RT_LBL:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_label_op();

    case InstrFormat::I_RS_LBL:
        return parse_register_op() && comma() && parse_label_op();

    case InstrFormat::I_RT_MEM:
        return parse_register_op() && comma() && parse_memory_op();

    case InstrFormat::J_LBL:
        return parse_label_op();

    case InstrFormat::REGIMM_RS_LBL:
        return parse_register_op() && comma() && parse_label_op();

    case InstrFormat::REGIMM_RS_IMM:
        return parse_register_op() && comma() && parse_signed_imm_op();
    }
    return record_error(peek().loc, StringView("internal: unknown instruction format"));
}

Bool Parser::parse_instr(InstrKind k, Stmt& s) {
    s.kind = Stmt::Kind::INSTR;
    s.sub  = static_cast<Uint32>(k);
    return parse_instr_operands(instr_format(k));
}

// -- Pseudo-instruction operands --------------------------------------

Bool Parser::parse_pseudo_operands(PseudoKind k) {
    auto comma = [&]() { return expect(TokenKind::COMMA, StringView("expected ','")); };

    switch (k) {
    case PseudoKind::NOP:
        return true;

    case PseudoKind::B:
    case PseudoKind::BAL:
        return parse_label_op();

    case PseudoKind::MOVE:
    case PseudoKind::NEG:
    case PseudoKind::NEGU:
    case PseudoKind::NOT:
        return parse_register_op() && comma() && parse_register_op();

    case PseudoKind::LI:
        return parse_register_op() && comma() && parse_signed_imm_op();

    case PseudoKind::LA:
        return parse_register_op() && comma() && parse_label_op();

    case PseudoKind::BEQZ:
    case PseudoKind::BNEZ:
        return parse_register_op() && comma() && parse_label_op();

    case PseudoKind::BLT:
    case PseudoKind::BLE:
    case PseudoKind::BGT:
    case PseudoKind::BGE:
        return parse_register_op() && comma()
            && parse_register_op() && comma()
            && parse_label_op();

    case PseudoKind::_COUNT:
        break;
    }
    return record_error(peek().loc, StringView("internal: unknown pseudo"));
}

Bool Parser::parse_pseudo(PseudoKind k, Stmt& s) {
    s.kind = Stmt::Kind::PSEUDO;
    s.sub  = static_cast<Uint32>(k);
    return parse_pseudo_operands(k);
}

// -- Directives -------------------------------------------------------

Bool Parser::parse_directive(DirectiveKind k, Stmt& s) {
    auto comma = [&]() { return expect(TokenKind::COMMA, StringView("expected ','")); };

    switch (k) {
    case DirectiveKind::TEXT:
    case DirectiveKind::DATA:
        s.kind = Stmt::Kind::SECTION;
        s.sub  = static_cast<Uint32>(k);
        return true;

    case DirectiveKind::WORD:
    case DirectiveKind::HALF:
    case DirectiveKind::BYTE: {
        switch (k) {
        case DirectiveKind::WORD: s.kind = Stmt::Kind::DATA_WORD; break;
        case DirectiveKind::HALF: s.kind = Stmt::Kind::DATA_HALF; break;
        case DirectiveKind::BYTE: s.kind = Stmt::Kind::DATA_BYTE; break;
        default: break;
        }
        // Comma-separated list of values: at least one required.
        const auto first_ok = (k == DirectiveKind::WORD)
            ? parse_imm_or_label_op()
            : parse_signed_imm_op();
        if (!first_ok) return false;
        while (peek().kind == TokenKind::COMMA) {
            advance();
            const auto more_ok = (k == DirectiveKind::WORD)
                ? parse_imm_or_label_op()
                : parse_signed_imm_op();
            if (!more_ok) return false;
        }
        return true;
    }

    case DirectiveKind::ASCII:
    case DirectiveKind::ASCIIZ: {
        const auto& t = peek();
        if (t.kind != TokenKind::STRING) {
            return record_error(t.loc, StringView("expected string literal"));
        }
        s.kind = (k == DirectiveKind::ASCII) ? Stmt::Kind::DATA_ASCII
            : Stmt::Kind::DATA_ASCIIZ;
        s.text = t.text; // raw content (escapes still encoded)
        advance();
        return true;
    }

    case DirectiveKind::SPACE: {
        s.kind = Stmt::Kind::DATA_SPACE;
        return parse_signed_imm_op();
    }

    case DirectiveKind::ALIGN: {
        s.kind = Stmt::Kind::ALIGN;
        return parse_signed_imm_op();
    }

    case DirectiveKind::GLOBL: {
        const auto& t = peek();
        if (t.kind != TokenKind::IDENT) {
            return record_error(t.loc, StringView("expected symbol name"));
        }
        s.kind = Stmt::Kind::GLOBL;
        s.text = t.text;
        advance();
        // Optional further symbols separated by commas.
        while (peek().kind == TokenKind::COMMA) {
            advance();
            if (peek().kind != TokenKind::IDENT) {
                return record_error(peek().loc, StringView("expected symbol name"));
            }
            advance();
        }
        return true;
    }

    case DirectiveKind::_COUNT:
        break;
    }
    return record_error(peek().loc, StringView("internal: unknown directive"));
}
