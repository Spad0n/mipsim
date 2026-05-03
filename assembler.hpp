#ifndef MIPS_ASSEMBLER_HPP
#define MIPS_ASSEMBLER_HPP

#include "parser.hpp"

static constexpr const Uint32 DEFAULT_BASE = 0x00400000u;

struct LabelEntry {
    StringView name;
    Uint32     addr;
    Location   loc;
};

/// Two-pass assembler: layout, then encode.
struct Assembler {
    Assembler(Allocator& alloc, Uint32 base_address = DEFAULT_BASE);

    Bool assemble(Slice<const Stmt> stmts, Slice<const Operand> operands);

    Slice<const Uint32>     bytecode()  const { return image_.slice(); }
    Slice<const LabelEntry> labels()    const { return labels_.slice(); }
    Slice<const Diagnostic> errors()    const { return errors_.slice(); }
    Bool                    has_errors() const { return !errors_.is_empty(); }

    Uint32 base_address() const { return base_; }

    Uint32 text_end_address() const { return base_ + text_size_; }

private:
    enum class Section : Uint32 { TEXT, DATA };

    // Pass 1: walk stmts to compute label addresses and segment sizes.
    Bool layout();

    // Pass 2: emit bytes for each stmt.
    Bool encode();

    // Pass-1 helpers: advance the current segment cursor.
    void advance_text(Uint32 n_words);
    void advance_data(Uint32 n_bytes);
    void align_data(Uint32 power_of_two);

    // Pass-2 emission — write to the proper segment.
    Bool emit_text_word(Uint32 word, Location loc);
    Bool emit_data_word(Uint32 word);
    Bool emit_data_byte(Uint8 byte);
    Bool emit_data_bytes(Slice<const Uint8> bytes);
    Bool pad_data_to_alignment(Uint32 power_of_two);

    // Encoding helpers.
    static Uint32 encode_R(Uint32 funct, Uint32 rs, Uint32 rt, Uint32 rd, Uint32 sa);
    static Uint32 encode_I(Uint32 opcode, Uint32 rs, Uint32 rt, Uint32 imm16);
    static Uint32 encode_J(Uint32 opcode, Uint32 target_addr);
    static Uint32 encode_REGIMM(Uint32 rs, Uint32 subop, Uint32 imm16);

    // Validation / range checks. All return false AND record an error
    // on failure.
    Bool check_imm_signed_16(Sint64 v, Location loc);
    Bool check_imm_unsigned_16(Sint64 v, Location loc);
    Bool check_imm_signed_or_unsigned_16(Sint64 v, Location loc);
    Bool check_shift_amount(Sint64 v, Location loc);
    Bool resolve_label(StringView name, Location loc, Uint32& out);
    Bool branch_offset(Uint32 from_pc, Uint32 target, Location loc, Uint32& imm16);
    Bool jump_target(Uint32 from_pc, Uint32 target, Location loc, Uint32& addr26);

    // Pseudo expansion. Pseudos are expanded to 1 or 2 real instructions.
    // pseudo_size_words() is used in pass 1 to advance the text cursor.
    Uint32 pseudo_size_words(PseudoKind k, Slice<const Operand> ops) const;
    Bool   encode_pseudo(PseudoKind k, Slice<const Operand> ops, Location loc);

    // Per-instruction encoding (pass 2).
    Bool encode_instr(InstrKind k, Slice<const Operand> ops, Location loc);

    // Operand fetch with strict kind checking.
    Bool require_reg(const Operand& op, Uint32& out);
    Bool require_imm(const Operand& op, Sint64& out);

    // Data-directive encoding (pass 2).
    Bool encode_data_word(Slice<const Operand> ops, Location loc);
    Bool encode_data_half(Slice<const Operand> ops, Location loc);
    Bool encode_data_byte(Slice<const Operand> ops, Location loc);
    Bool encode_data_string(StringView text, Bool zero_term, Location loc);
    Bool decode_string_byte(StringView text, Ulen& i, Uint32& out, Location loc);

    Bool record_error(Location loc, StringView msg);

    Slice<const Stmt>    stmts_;
    Slice<const Operand> operands_;

    Allocator&        alloc_;
    Uint32            base_;
    Uint32            text_size_  = 0;
    Uint32            data_base_  = 0;
    Uint32            data_size_  = 0;

    // Pass 2 state — current PC for the instruction being emitted, and
    // current data-segment cursor (relative to data_base_).
    Uint32            text_pc_    = 0;
    Uint32            data_off_   = 0;

    Array<LabelEntry> labels_;
    Array<Uint32>     image_;       // final concatenated program
    Array<Diagnostic> errors_;
};

#endif // MIPS_ASSEMBLER_HPP
