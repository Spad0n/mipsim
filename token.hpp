#ifndef MIPS_TOKEN_HPP
#define MIPS_TOKEN_HPP

#include "ctl/string.hpp"

using namespace ctl;

enum class TokenKind : Uint32 {
#define TOK_KIND(e, ...) e,
#include "tokens.inl"
    _COUNT,
};

enum class InstrKind : Uint32 {
#define TOK_INSTR(e, ...) e,
#include "tokens.inl"
    _COUNT,
};

enum class PseudoKind : Uint32 {
#define TOK_PSEUDO(e, ...) e,
#include "tokens.inl"
    _COUNT,
};

enum class DirectiveKind : Uint32 {
#define TOK_DIRECTIVE(e, ...) e,
#include "tokens.inl"
    _COUNT,
};

enum class InstrFormat : Uint32 {
    // R-type: major opcode = 0, code = funct
    R_NONE,         // syscall, break
    R_RS,           // jr, mthi, mtlo
    R_RD,           // mfhi, mflo
    R_RD_RS,        // jalr (with explicit rd)
    R_RS_RT,        // mult, multu, div, divu, traps
    R_RD_RS_RT,     // add, addu, sub, ..., movz, movn
    R_RD_RT_SA,     // sll, srl, sra (immediate shift amount)
    R_RD_RT_RS,     // sllv, srlv, srav (variable shift)

    // I-type: code = opcode
    I_RT_RS_IMM,    // addi, addiu, slti, sltiu, andi, ori, xori
    I_RT_IMM,       // lui
    I_RS_RT_LBL,    // beq, bne, beql, bnel
    I_RS_LBL,       // blez, bgtz, blezl, bgtzl
    I_RT_MEM,       // lw $rt, off($rs); sw, lb, sb, ...

    // J-type: code = opcode
    J_LBL,          // j, jal

    // REGIMM: major opcode = 1, code = rt sub-op
    REGIMM_RS_LBL,  // bltz, bgez, ..., bltzal, bgezal
    REGIMM_RS_IMM,  // tgei, tlti, tltiu, teqi, tnei
};

StringView token_kind_name(TokenKind k);

StringView instr_mnemonic(InstrKind k);

InstrFormat instr_format(InstrKind k);

Uint32 instr_code(InstrKind k);

StringView pseudo_mnemonic(PseudoKind k);
StringView directive_mnemonic(DirectiveKind k);

Bool lookup_register(StringView name, Uint32& out);

Bool lookup_instr(StringView name, InstrKind& out);

Bool lookup_pseudo(StringView name, PseudoKind& out);

Bool lookup_directive(StringView name, DirectiveKind& out);

struct Location {
    Uint32 line   = 1;
    Uint32 column = 1;
    Uint32 offset = 0;
};

struct Token {
    TokenKind  kind = TokenKind::INVALID;
    Uint32     sub  = 0;
    Location   loc;
    StringView text;
    Uint64     int_value = 0;

    CTL_FORCEINLINE InstrKind     as_instr()     const { return static_cast<InstrKind>(sub);     }
    CTL_FORCEINLINE PseudoKind    as_pseudo()    const { return static_cast<PseudoKind>(sub);    }
    CTL_FORCEINLINE DirectiveKind as_directive() const { return static_cast<DirectiveKind>(sub); }
};

#endif // MIPS_TOKEN_HPP
