#ifndef MIPS_PARSER_HPP
#define MIPS_PARSER_HPP

#include "ctl/array.hpp"
#include "ctl/maybe.hpp"
#include "lexer.hpp"

using namespace ctl;

struct Operand {
    enum class Kind { REG, IMM, LABEL, MEM };
    Kind       kind = Kind::IMM;
    Uint32     reg  = 0;
    Sint64     imm  = 0;
    StringView label;
    Location   loc;
};

struct Stmt {
    enum class Kind : Uint32 {
        LABEL_DEF,    // text holds the label name
        INSTR,        // sub = InstrKind, operands per InstrFormat
        PSEUDO,       // sub = PseudoKind
        SECTION,      // sub = DirectiveKind (TEXT or DATA)
        DATA_WORD,    // operands = list of IMM / LABEL values
        DATA_HALF,    // operands = list of IMM values
        DATA_BYTE,    // operands = list of IMM values
        DATA_ASCII,   // text = raw string content (escapes still encoded)
        DATA_ASCIIZ,
        DATA_SPACE,   // operands[0].imm = byte count
        ALIGN,        // operands[0].imm = alignment exponent (1<<n)
        GLOBL,        // text = symbol name (informational, no code)
    };
    Kind        kind      = Kind::LABEL_DEF;
    Uint32      sub       = 0;
    Location    loc;
    StringView  text;
    Uint32      op_offset = 0;
    Uint32      op_count  = 0;
};

struct Diagnostic {
    Location   loc;
    StringView message;
};

struct Parser {
    Parser(Slice<Token> tokens, Allocator& alloc);

    Bool parse();

    Slice<const Stmt>       stmts()     const { return stmts_.slice(); }
    Slice<const Operand>    operands()  const { return operands_.slice(); }
    Slice<const Diagnostic> errors()    const { return errors_.slice(); }
    Bool                    has_errors() const { return !errors_.is_empty(); }

private:
    const Token& peek(Ulen offset = 0) const;
    Bool         at_end() const;
    void         advance();
    Bool         match(TokenKind k);
    Bool         expect(TokenKind k, StringView what);

    Bool record_error(Location loc, StringView msg);
    void skip_newlines();
    void recover_to_newline();

    Bool parse_one();
    Bool parse_label_def();
    Bool parse_instr(InstrKind k, Stmt& s);
    Bool parse_pseudo(PseudoKind k, Stmt& s);
    Bool parse_directive(DirectiveKind k, Stmt& s);

    // Operand-level parsers — return false on error AND record a
    // diagnostic; the operand is pushed to operands_ on success.
    Bool parse_instr_operands(InstrFormat fmt);
    Bool parse_pseudo_operands(PseudoKind k);

    Bool parse_register_op();
    Bool parse_signed_imm_op();
    Bool parse_imm_or_label_op();
    Bool parse_label_op();
    Bool parse_memory_op();
    Bool parse_signed_integer(Sint64& out, Location& loc);

    Bool finalize_stmt(Stmt& s, Uint32 op_start);

    Slice<Token> tokens_;
    Allocator&         alloc_;
    Ulen               pos_ = 0;
    Array<Stmt>        stmts_;
    Array<Operand>     operands_;
    Array<Diagnostic>  errors_;
};

#endif // MIPS_PARSER_HPP
