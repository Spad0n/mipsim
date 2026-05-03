#include "assembler.hpp"
#include "ctl/move.hpp"

Assembler::Assembler(Allocator& alloc, Uint32 base_address)
    : alloc_{alloc}
    , base_{base_address}
    , labels_{alloc}
    , image_{alloc}
    , errors_{alloc}
{}

Bool Assembler::record_error(Location loc, StringView msg) {
    Diagnostic d{ loc, msg };
    (void)errors_.push_back(move(d));
    return false;
}

Uint32 Assembler::encode_R(Uint32 funct, Uint32 rs, Uint32 rt, Uint32 rd, Uint32 sa) {
    // [opcode:6=0][rs:5][rt:5][rd:5][sa:5][funct:6]
    return ((rs & 0x1Fu) << 21)
        | ((rt & 0x1Fu) << 16)
        | ((rd & 0x1Fu) << 11)
        | ((sa & 0x1Fu) <<  6)
        |  (funct & 0x3Fu);
}

Uint32 Assembler::encode_I(Uint32 opcode, Uint32 rs, Uint32 rt, Uint32 imm16) {
    // [opcode:6][rs:5][rt:5][imm:16]
    return ((opcode & 0x3Fu) << 26)
        | ((rs & 0x1Fu) << 21)
        | ((rt & 0x1Fu) << 16)
        |  (imm16 & 0xFFFFu);
}

Uint32 Assembler::encode_J(Uint32 opcode, Uint32 target_addr) {
    // [opcode:6][address:26]; the VM reconstructs PC = (PC & 0xF0000000) | (addr<<2)
    return ((opcode & 0x3Fu) << 26)
        | ((target_addr >> 2) & 0x03FFFFFFu);
}

Uint32 Assembler::encode_REGIMM(Uint32 rs, Uint32 subop, Uint32 imm16) {
    // [opcode:6=1][rs:5][subop:5][imm:16]
    return (1u << 26)
        | ((rs    & 0x1Fu) << 21)
        | ((subop & 0x1Fu) << 16)
        |  (imm16 & 0xFFFFu);
}

Bool Assembler::check_imm_signed_16(Sint64 v, Location loc) {
    if (v < -32768 || v > 32767) {
        return record_error(loc, StringView("immediate out of range (16-bit signed)"));
    }
    return true;
}

Bool Assembler::check_imm_unsigned_16(Sint64 v, Location loc) {
    if (v < 0 || v > 65535) {
        return record_error(loc, StringView("immediate out of range (16-bit unsigned)"));
    }
    return true;
}

Bool Assembler::check_imm_signed_or_unsigned_16(Sint64 v, Location loc) {
    if (v < -32768 || v > 65535) {
        return record_error(loc, StringView("immediate out of range (16 bits)"));
    }
    return true;
}

Bool Assembler::check_shift_amount(Sint64 v, Location loc) {
    if (v < 0 || v > 31) {
        return record_error(loc, StringView("shift amount out of range (0..31)"));
    }
    return true;
}

Bool Assembler::resolve_label(StringView name, Location loc, Uint32& out) {
    for (const auto& e : labels_) {
        if (e.name == name) { out = e.addr; return true; }
    }
    return record_error(loc, StringView("undefined label"));
}

Bool Assembler::branch_offset(Uint32 from_pc, Uint32 target, Location loc, Uint32& imm16) {
    // imm = (target - (from_pc + 4)) >> 2; must fit in 16-bit signed.
    const auto delta = static_cast<Sint64>(target) - static_cast<Sint64>(from_pc + 4);
    if ((delta & 3) != 0) {
        return record_error(loc, StringView("branch target not 4-byte aligned"));
    }
    const auto words = delta >> 2;
    if (words < -32768 || words > 32767) {
        return record_error(loc, StringView("branch target out of range (16-bit signed offset)"));
    }
    imm16 = static_cast<Uint32>(static_cast<Sint32>(words)) & 0xFFFFu;
    return true;
}

Bool Assembler::jump_target(Uint32 from_pc, Uint32 target, Location loc, Uint32& addr26) {
    if ((target & 3) != 0) {
        return record_error(loc, StringView("jump target not 4-byte aligned"));
    }
    // Jumps preserve the upper 4 bits of PC. Target must be in the same
    // 256MB region as the current PC.
    if ((target & 0xF0000000u) != ((from_pc + 4) & 0xF0000000u)) {
        return record_error(loc, StringView("jump target outside current 256MB region"));
    }
    addr26 = (target >> 2) & 0x03FFFFFFu;
    return true;
}

Bool Assembler::require_reg(const Operand& op, Uint32& out) {
    if (op.kind != Operand::Kind::REG) {
        return record_error(op.loc, StringView("expected register operand"));
    }
    out = op.reg;
    return true;
}

Bool Assembler::require_imm(const Operand& op, Sint64& out) {
    if (op.kind != Operand::Kind::IMM) {
        return record_error(op.loc, StringView("expected immediate operand"));
    }
    out = op.imm;
    return true;
}

Uint32 Assembler::pseudo_size_words(PseudoKind k, Slice<const Operand> ops) const {
    switch (k) {
    case PseudoKind::NOP:
    case PseudoKind::MOVE:
    case PseudoKind::B:
    case PseudoKind::BAL:
    case PseudoKind::BEQZ:
    case PseudoKind::BNEZ:
    case PseudoKind::NEG:
    case PseudoKind::NEGU:
    case PseudoKind::NOT:
        return 1;
    case PseudoKind::LI:
        // 1 word if the value fits in 16 bits (signed or unsigned), else 2.
        if (ops.length() >= 2 && ops[1].kind == Operand::Kind::IMM) {
            const auto v = ops[1].imm;
            if (v >= -32768 && v <= 65535) return 1;
        }
        return 2;
    case PseudoKind::LA:
        return 2; // always lui + ori
    case PseudoKind::BLT:
    case PseudoKind::BLE:
    case PseudoKind::BGT:
    case PseudoKind::BGE:
        return 2;
    case PseudoKind::_COUNT:
        break;
    }
    return 1;
}

// -- Layout (pass 1) --------------------------------------------------

static Uint32 align_up(Uint32 v, Uint32 a) {
    if (a <= 1) return v;
    return (v + (a - 1)) & ~(a - 1);
}

// Decode a single byte from a string-literal text body that still
// contains escape sequences. Advances `i` past the consumed characters.
Bool Assembler::decode_string_byte(StringView text, Ulen& i, Uint32& out, Location loc) {
    if (i >= text.length()) {
        return record_error(loc, StringView("internal: end of string"));
    }
    const auto c = text[i];
    if (c != '\\') {
        out = static_cast<Uint8>(c);
        i++;
        return true;
    }
    i++;
    if (i >= text.length()) {
        return record_error(loc, StringView("incomplete escape sequence"));
    }
    const auto e = text[i++];
    switch (e) {
    case 'n':  out = '\n'; return true;
    case 'r':  out = '\r'; return true;
    case 't':  out = '\t'; return true;
    case '0':  out = 0;    return true;
    case '\\': out = '\\'; return true;
    case '\'': out = '\''; return true;
    case '"':  out = '"';  return true;
    case 'x': {
        if (i + 2 > text.length()) {
            return record_error(loc, StringView("incomplete \\x escape"));
        }
        auto hv = [](char ch) -> Sint32 {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            return -1;
        };
        const auto h0 = hv(text[i]);
        const auto h1 = hv(text[i + 1]);
        if (h0 < 0 || h1 < 0) {
            return record_error(loc, StringView("invalid hex digit in \\x escape"));
        }
        out = static_cast<Uint32>((h0 << 4) | h1);
        i += 2;
        return true;
    }
    }
    return record_error(loc, StringView("unknown escape sequence"));
}

// Compute the number of bytes that a string literal will occupy after
// escape-decoding. Returns false (without recording an error) if the
// string contains something malformed; pass 2 will produce the proper
// diagnostic at that point.
static Uint32 decoded_byte_count(StringView text) {
    Uint32 n = 0;
    Ulen i = 0;
    while (i < text.length()) {
        if (text[i] == '\\' && i + 1 < text.length()) {
            if (text[i + 1] == 'x') {
                if (i + 4 <= text.length()) { i += 4; n++; continue; }
                return n;
            }
            i += 2; n++;
            continue;
        }
        i++; n++;
    }
    return n;
}

Bool Assembler::layout() {
    struct Pending { StringView name; Location loc; };
    Array<Pending> pending{alloc_};

    Section section = Section::TEXT;
    Uint32  text_off = 0;
    Uint32  data_off = 0;

    auto cur = [&](Uint32*& p) { p = (section == Section::TEXT) ? &text_off : &data_off; };

    auto commit_labels = [&](Uint32 off) -> Bool {
        for (const auto& p : pending) {
            LabelEntry e;
            e.name = p.name;
            e.loc  = p.loc;
            e.addr = (section == Section::TEXT) ? (base_ + off) : off; // data: temporary, patched later
            // We need to know if this is text or data when patching, so
            // store a marker by using addr=offset for data and addr=base+off for text.
            // After we know data_base_, patch data labels by adding data_base_.
            // To distinguish, we record the section using a tag-bit trick
            // by stashing an out-of-band flag. Simpler: keep two arrays.
            if (!labels_.push_back(move(e))) return false;
        }
        pending.clear();
        return true;
    };

    // We need to know per label whether it is text or data so we can
    // patch addresses after layout. Rather than cramming a bit into
    // `addr`, keep a parallel array of section tags.
    Array<Uint8> label_section_tags{alloc_};

    auto commit_labels_v2 = [&](Uint32 off) -> Bool {
        for (const auto& p : pending) {
            LabelEntry e;
            e.name = p.name;
            e.loc  = p.loc;
            e.addr = off; // section-relative for now
            if (!labels_.push_back(move(e)))             return false;
            if (!label_section_tags.push_back(static_cast<Uint8>(section))) return false;
        }
        pending.clear();
        return true;
    };
    (void)commit_labels;

    for (Ulen i = 0; i < stmts_.length(); i++) {
        const auto& s   = stmts_[i];
        const auto  ops = operands_.subrange(s.op_offset, s.op_offset + s.op_count);

        if (s.kind == Stmt::Kind::LABEL_DEF) {
            Pending p{ s.text, s.loc };
            if (!pending.push_back(move(p))) return false;
            continue;
        }

        // Required alignment for the upcoming bytes.
        Uint32 req = 1;
        switch (s.kind) {
        case Stmt::Kind::INSTR:
        case Stmt::Kind::PSEUDO:
        case Stmt::Kind::DATA_WORD:
            req = 4; break;
        case Stmt::Kind::DATA_HALF:
            req = 2; break;
        default:
            req = 1; break;
        }

        Uint32* p = nullptr; cur(p);
        if (req > 1) *p = align_up(*p, req);

        // ALIGN directive: align cursor to (1 << operand).
        if (s.kind == Stmt::Kind::ALIGN) {
            if (ops.length() == 1 && ops[0].kind == Operand::Kind::IMM) {
                const auto exp = ops[0].imm;
                if (exp < 0 || exp > 16) {
                    record_error(s.loc, StringView(".align argument out of range"));
                } else {
                    *p = align_up(*p, 1u << static_cast<Uint32>(exp));
                }
            }
        }

        if (!commit_labels_v2(*p)) return false;

        switch (s.kind) {
        case Stmt::Kind::LABEL_DEF: break; // handled above
        case Stmt::Kind::INSTR:
            if (section != Section::TEXT) {
                record_error(s.loc, StringView("instruction outside .text"));
            }
            *p += 4;
            break;
        case Stmt::Kind::PSEUDO:
            if (section != Section::TEXT) {
                record_error(s.loc, StringView("pseudo instruction outside .text"));
            }
            *p += 4 * pseudo_size_words(static_cast<PseudoKind>(s.sub), ops);
            break;
        case Stmt::Kind::SECTION:
            section = (s.sub == static_cast<Uint32>(DirectiveKind::TEXT))
                ? Section::TEXT : Section::DATA;
            break;
        case Stmt::Kind::DATA_WORD:
            *p += 4 * static_cast<Uint32>(ops.length());
            break;
        case Stmt::Kind::DATA_HALF:
            *p += 2 * static_cast<Uint32>(ops.length());
            break;
        case Stmt::Kind::DATA_BYTE:
            *p += static_cast<Uint32>(ops.length());
            break;
        case Stmt::Kind::DATA_ASCII:
            *p += decoded_byte_count(s.text);
            break;
        case Stmt::Kind::DATA_ASCIIZ:
            *p += decoded_byte_count(s.text) + 1;
            break;
        case Stmt::Kind::DATA_SPACE:
            if (ops.length() >= 1 && ops[0].kind == Operand::Kind::IMM) {
                const auto n = ops[0].imm;
                if (n < 0) {
                    record_error(s.loc, StringView(".space argument must be non-negative"));
                } else {
                    *p += static_cast<Uint32>(n);
                }
            }
            break;
        case Stmt::Kind::ALIGN:   break; // already applied above
        case Stmt::Kind::GLOBL:   break; // no-op
        }
    }

    // Trailing labels (point to end of their section).
    Uint32* tail_p = (section == Section::TEXT) ? &text_off : &data_off;
    if (!commit_labels_v2(*tail_p)) return false;

    // Compute final layout.
    text_size_ = align_up(text_off, 4);
    data_size_ = data_off;
    data_base_ = base_ + text_size_;

    // Patch label addresses now that we know data_base_.
    for (Ulen i = 0; i < labels_.length(); i++) {
        const auto sec = static_cast<Section>(label_section_tags[i]);
        if (sec == Section::TEXT) {
            labels_[i].addr = base_ + labels_[i].addr;
        } else {
            labels_[i].addr = data_base_ + labels_[i].addr;
        }
    }

    // Detect duplicate labels.
    for (Ulen i = 0; i < labels_.length(); i++) {
        for (Ulen j = i + 1; j < labels_.length(); j++) {
            if (labels_[i].name == labels_[j].name) {
                record_error(labels_[j].loc, StringView("duplicate label"));
            }
        }
    }

    return true;
}

// -- Pass 2 emission helpers ------------------------------------------

Bool Assembler::emit_text_word(Uint32 word, Location loc) {
    const auto idx = (text_pc_ - base_) / 4;
    if (idx >= image_.length()) {
        return record_error(loc, StringView("internal: text overflow"));
    }
    image_[idx] = word;
    text_pc_ += 4;
    return true;
}

Bool Assembler::emit_data_byte(Uint8 byte) {
    const auto word_idx = (text_size_ / 4) + (data_off_ / 4);
    if (word_idx >= image_.length()) return false;
    const auto shift = (3u - (data_off_ & 3u)) * 8u;
    image_[word_idx] = (image_[word_idx] & ~(0xFFu << shift))
        | ((static_cast<Uint32>(byte) & 0xFFu) << shift);
    data_off_++;
    return true;
}

Bool Assembler::emit_data_word(Uint32 word) {
    if ((data_off_ & 3) != 0) return false;
    const auto word_idx = (text_size_ / 4) + (data_off_ / 4);
    if (word_idx >= image_.length()) return false;
    image_[word_idx] = word;
    data_off_ += 4;
    return true;
}

Bool Assembler::emit_data_bytes(Slice<const Uint8> bytes) {
    for (const auto b : bytes) {
        if (!emit_data_byte(b)) return false;
    }
    return true;
}

Bool Assembler::pad_data_to_alignment(Uint32 power_of_two) {
    const auto target = align_up(data_off_, power_of_two);
    while (data_off_ < target) {
        if (!emit_data_byte(0)) return false;
    }
    return true;
}

Bool Assembler::encode_instr(InstrKind k, Slice<const Operand> ops, Location loc) {
    const auto fmt  = instr_format(k);
    const auto code = instr_code(k);
    const auto pc   = text_pc_;

    Uint32 word = 0;

    auto need = [&](Ulen n) -> Bool {
        if (ops.length() < n) {
            return record_error(loc, StringView("not enough operands"));
        }
        return true;
    };

    switch (fmt) {
    case InstrFormat::R_NONE: {
        word = encode_R(code, 0, 0, 0, 0);
        break;
    }
    case InstrFormat::R_RS: {
        if (!need(1)) return true;
        Uint32 rs;
        if (!require_reg(ops[0], rs)) return true;
        word = encode_R(code, rs, 0, 0, 0);
        break;
    }
    case InstrFormat::R_RD: {
        if (!need(1)) return true;
        Uint32 rd;
        if (!require_reg(ops[0], rd)) return true;
        word = encode_R(code, 0, 0, rd, 0);
        break;
    }
    case InstrFormat::R_RD_RS: {
        if (!need(2)) return true;
        Uint32 rd, rs;
        if (!require_reg(ops[0], rd)) return true;
        if (!require_reg(ops[1], rs)) return true;
        word = encode_R(code, rs, 0, rd, 0);
        break;
    }
    case InstrFormat::R_RS_RT: {
        if (!need(2)) return true;
        Uint32 rs, rt;
        if (!require_reg(ops[0], rs)) return true;
        if (!require_reg(ops[1], rt)) return true;
        word = encode_R(code, rs, rt, 0, 0);
        break;
    }
    case InstrFormat::R_RD_RS_RT: {
        if (!need(3)) return true;
        Uint32 rd, rs, rt;
        if (!require_reg(ops[0], rd)) return true;
        if (!require_reg(ops[1], rs)) return true;
        if (!require_reg(ops[2], rt)) return true;
        word = encode_R(code, rs, rt, rd, 0);
        break;
    }
    case InstrFormat::R_RD_RT_SA: {
        if (!need(3)) return true;
        Uint32 rd, rt;
        Sint64 sa;
        if (!require_reg(ops[0], rd)) return true;
        if (!require_reg(ops[1], rt)) return true;
        if (!require_imm(ops[2], sa)) return true;
        if (!check_shift_amount(sa, ops[2].loc)) return true;
        word = encode_R(code, 0, rt, rd, static_cast<Uint32>(sa));
        break;
    }
    case InstrFormat::R_RD_RT_RS: {
        if (!need(3)) return true;
        Uint32 rd, rt, rs;
        if (!require_reg(ops[0], rd)) return true;
        if (!require_reg(ops[1], rt)) return true;
        if (!require_reg(ops[2], rs)) return true;
        word = encode_R(code, rs, rt, rd, 0);
        break;
    }
    case InstrFormat::I_RT_RS_IMM: {
        if (!need(3)) return true;
        Uint32 rt, rs;
        Sint64 imm;
        if (!require_reg(ops[0], rt)) return true;
        if (!require_reg(ops[1], rs)) return true;
        if (!require_imm(ops[2], imm)) return true;
        // andi/ori/xori treat as unsigned; addi/addiu/slti/sltiu treat as signed.
        // We accept any 16-bit fit and truncate. Range-check the union.
        if (!check_imm_signed_or_unsigned_16(imm, ops[2].loc)) return true;
        word = encode_I(code, rs, rt, static_cast<Uint32>(imm) & 0xFFFFu);
        break;
    }
    case InstrFormat::I_RT_IMM: {
        if (!need(2)) return true;
        Uint32 rt;
        Sint64 imm;
        if (!require_reg(ops[0], rt)) return true;
        if (!require_imm(ops[1], imm)) return true;
        if (!check_imm_signed_or_unsigned_16(imm, ops[1].loc)) return true;
        word = encode_I(code, 0, rt, static_cast<Uint32>(imm) & 0xFFFFu);
        break;
    }
    case InstrFormat::I_RS_RT_LBL: {
        if (!need(3)) return true;
        Uint32 rs, rt;
        if (!require_reg(ops[0], rs)) return true;
        if (!require_reg(ops[1], rt)) return true;
        if (ops[2].kind != Operand::Kind::LABEL) {
            record_error(ops[2].loc, StringView("expected label"));
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[2].label, ops[2].loc, target)) return true;
        Uint32 imm16;
        if (!branch_offset(pc, target, ops[2].loc, imm16)) return true;
        word = encode_I(code, rs, rt, imm16);
        break;
    }
    case InstrFormat::I_RS_LBL: {
        if (!need(2)) return true;
        Uint32 rs;
        if (!require_reg(ops[0], rs)) return true;
        if (ops[1].kind != Operand::Kind::LABEL) {
            record_error(ops[1].loc, StringView("expected label"));
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[1].label, ops[1].loc, target)) return true;
        Uint32 imm16;
        if (!branch_offset(pc, target, ops[1].loc, imm16)) return true;
        word = encode_I(code, rs, 0, imm16);
        break;
    }
    case InstrFormat::I_RT_MEM: {
        if (!need(2)) return true;
        Uint32 rt;
        if (!require_reg(ops[0], rt)) return true;
        if (ops[1].kind != Operand::Kind::MEM) {
            record_error(ops[1].loc, StringView("expected 'imm(reg)' memory operand"));
            return true;
        }
        const auto rs  = ops[1].reg;
        const auto off = ops[1].imm;
        if (!check_imm_signed_16(off, ops[1].loc)) return true;
        word = encode_I(code, rs, rt, static_cast<Uint32>(off) & 0xFFFFu);
        break;
    }
    case InstrFormat::J_LBL: {
        if (!need(1)) return true;
        if (ops[0].kind != Operand::Kind::LABEL) {
            record_error(ops[0].loc, StringView("expected label"));
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[0].label, ops[0].loc, target)) return true;
        Uint32 addr26;
        if (!jump_target(pc, target, ops[0].loc, addr26)) return true;
        word = ((code & 0x3Fu) << 26) | addr26;
        break;
    }
    case InstrFormat::REGIMM_RS_LBL: {
        if (!need(2)) return true;
        Uint32 rs;
        if (!require_reg(ops[0], rs)) return true;
        if (ops[1].kind != Operand::Kind::LABEL) {
            record_error(ops[1].loc, StringView("expected label"));
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[1].label, ops[1].loc, target)) return true;
        Uint32 imm16;
        if (!branch_offset(pc, target, ops[1].loc, imm16)) return true;
        word = encode_REGIMM(rs, code, imm16);
        break;
    }
    case InstrFormat::REGIMM_RS_IMM: {
        if (!need(2)) return true;
        Uint32 rs;
        Sint64 imm;
        if (!require_reg(ops[0], rs)) return true;
        if (!require_imm(ops[1], imm)) return true;
        if (!check_imm_signed_16(imm, ops[1].loc)) return true;
        word = encode_REGIMM(rs, code, static_cast<Uint32>(imm) & 0xFFFFu);
        break;
    }
    }

    emit_text_word(word, loc);
    return true;
}

// -- Pseudo-instruction expansion -------------------------------------
// TODO: need to implement the rest of pseudo instruction

Bool Assembler::encode_pseudo(PseudoKind k, Slice<const Operand> ops, Location loc) {
    // Encoding constants for the reals we expand into. Keep these
    // in sync with tokens.def.
    constexpr Uint32 FUNCT_SLL     = 0x00;
    constexpr Uint32 FUNCT_ADDU    = 0x21;
    constexpr Uint32 FUNCT_SUB     = 0x22;
    constexpr Uint32 FUNCT_SUBU    = 0x23;
    constexpr Uint32 FUNCT_NOR     = 0x27;
    constexpr Uint32 FUNCT_SLT     = 0x2A;
    constexpr Uint32 OP_BEQ        = 0x04;
    constexpr Uint32 OP_BNE        = 0x05;
    constexpr Uint32 OP_ADDIU      = 0x09;
    constexpr Uint32 OP_ORI        = 0x0D;
    constexpr Uint32 OP_LUI        = 0x0F;
    constexpr Uint32 REGIMM_BGEZAL = 0x11;
    constexpr Uint32 REG_AT        = 1;
    constexpr Uint32 REG_ZERO      = 0;

    switch (k) {
    case PseudoKind::NOP: {
        // sll $0, $0, 0
        emit_text_word(encode_R(FUNCT_SLL, 0, 0, 0, 0), loc);
        return true;
    }
    case PseudoKind::MOVE: {
        if (ops.length() < 2) return record_error(loc, StringView("move: missing operand"));
        Uint32 rd, rs;
        if (!require_reg(ops[0], rd) || !require_reg(ops[1], rs)) {
            emit_text_word(0, loc);
            return true;
        }
        // addu rd, rs, $0
        emit_text_word(encode_R(FUNCT_ADDU, rs, REG_ZERO, rd, 0), loc);
        return true;
    }
    case PseudoKind::NEG:
    case PseudoKind::NEGU: {
        if (ops.length() < 2) return record_error(loc, StringView("neg: missing operand"));
        Uint32 rd, rs;
        if (!require_reg(ops[0], rd) || !require_reg(ops[1], rs)) {
            emit_text_word(0, loc);
            return true;
        }
        const auto funct = (k == PseudoKind::NEG) ? FUNCT_SUB : FUNCT_SUBU;
        emit_text_word(encode_R(funct, REG_ZERO, rs, rd, 0), loc);
        return true;
    }
    case PseudoKind::NOT: {
        if (ops.length() < 2) return record_error(loc, StringView("not: missing operand"));
        Uint32 rd, rs;
        if (!require_reg(ops[0], rd) || !require_reg(ops[1], rs)) {
            emit_text_word(0, loc);
            return true;
        }
        // nor rd, rs, $0
        emit_text_word(encode_R(FUNCT_NOR, rs, REG_ZERO, rd, 0), loc);
        return true;
    }
    case PseudoKind::LI: {
        if (ops.length() < 2) return record_error(loc, StringView("li: missing operand"));
        Uint32 rt;
        Sint64 v;
        if (!require_reg(ops[0], rt) || !require_imm(ops[1], v)) {
            emit_text_word(0, loc);
            return true;
        }
        // Match the size we promised in pseudo_size_words().
        if (v >= -32768 && v <= 32767) {
            // addiu rt, $0, v
            emit_text_word(encode_I(OP_ADDIU, REG_ZERO, rt,
                                    static_cast<Uint32>(v) & 0xFFFFu), loc);
        } else if (v >= 0 && v <= 65535) {
            // ori rt, $0, v
            emit_text_word(encode_I(OP_ORI, REG_ZERO, rt,
                                    static_cast<Uint32>(v) & 0xFFFFu), loc);
        } else {
            // 64-bit checks here are loose; we accept anything
            // representable in 32 bits (signed or unsigned).
            if (v < -(Sint64(1) << 31) || v > (Sint64(1) << 32) - 1) {
                return record_error(ops[1].loc, StringView("li value out of 32-bit range"));
            }
            const auto vu = static_cast<Uint32>(static_cast<Uint64>(v));
            const auto hi = (vu >> 16) & 0xFFFFu;
            const auto lo = vu & 0xFFFFu;
            // lui rt, hi
            emit_text_word(encode_I(OP_LUI, REG_ZERO, rt, hi), loc);
            // ori rt, rt, lo
            emit_text_word(encode_I(OP_ORI, rt, rt, lo), loc);
        }
        return true;
    }
    case PseudoKind::LA: {
        if (ops.length() < 2) return record_error(loc, StringView("la: missing operand"));
        Uint32 rt;
        if (!require_reg(ops[0], rt)) {
            emit_text_word(0, loc); emit_text_word(0, loc);
            return true;
        }
        if (ops[1].kind != Operand::Kind::LABEL) {
            record_error(ops[1].loc, StringView("la: expected label"));
            emit_text_word(0, loc); emit_text_word(0, loc);
            return true;
        }
        Uint32 addr;
        if (!resolve_label(ops[1].label, ops[1].loc, addr)) {
            emit_text_word(0, loc); emit_text_word(0, loc);
            return true;
        }
        const auto hi = (addr >> 16) & 0xFFFFu;
        const auto lo =  addr        & 0xFFFFu;
        emit_text_word(encode_I(OP_LUI, REG_ZERO, rt, hi), loc);
        emit_text_word(encode_I(OP_ORI, rt,       rt, lo), loc);
        return true;
    }
    case PseudoKind::B: {
        // beq $0, $0, label
        if (ops.length() < 1 || ops[0].kind != Operand::Kind::LABEL) {
            record_error(loc, StringView("b: expected label"));
            emit_text_word(0, loc);
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[0].label, ops[0].loc, target)) {
            emit_text_word(0, loc); return true;
        }
        Uint32 imm16;
        if (!branch_offset(text_pc_, target, ops[0].loc, imm16)) {
            emit_text_word(0, loc); return true;
        }
        emit_text_word(encode_I(OP_BEQ, REG_ZERO, REG_ZERO, imm16), loc);
        return true;
    }
    case PseudoKind::BAL: {
        // bgezal $0, label
        if (ops.length() < 1 || ops[0].kind != Operand::Kind::LABEL) {
            record_error(loc, StringView("bal: expected label"));
            emit_text_word(0, loc);
            return true;
        }
        Uint32 target;
        if (!resolve_label(ops[0].label, ops[0].loc, target)) {
            emit_text_word(0, loc); return true;
        }
        Uint32 imm16;
        if (!branch_offset(text_pc_, target, ops[0].loc, imm16)) {
            emit_text_word(0, loc); return true;
        }
        emit_text_word(encode_REGIMM(REG_ZERO, REGIMM_BGEZAL, imm16), loc);
        return true;
    }
    case PseudoKind::BEQZ:
    case PseudoKind::BNEZ: {
        if (ops.length() < 2) return record_error(loc, StringView("beqz/bnez: missing operand"));
        Uint32 rs;
        if (!require_reg(ops[0], rs)) { emit_text_word(0, loc); return true; }
        if (ops[1].kind != Operand::Kind::LABEL) {
            record_error(ops[1].loc, StringView("expected label"));
            emit_text_word(0, loc); return true;
        }
        Uint32 target;
        if (!resolve_label(ops[1].label, ops[1].loc, target)) {
            emit_text_word(0, loc); return true;
        }
        Uint32 imm16;
        if (!branch_offset(text_pc_, target, ops[1].loc, imm16)) {
            emit_text_word(0, loc); return true;
        }
        const auto op = (k == PseudoKind::BEQZ) ? OP_BEQ : OP_BNE;
        emit_text_word(encode_I(op, rs, REG_ZERO, imm16), loc);
        return true;
    }
    case PseudoKind::BLT:
    case PseudoKind::BLE:
    case PseudoKind::BGT:
    case PseudoKind::BGE: {
        if (ops.length() < 3) return record_error(loc, StringView("missing operand"));
        Uint32 rs, rt;
        if (!require_reg(ops[0], rs) || !require_reg(ops[1], rt)) {
            emit_text_word(0, loc); emit_text_word(0, loc);
            return true;
        }
        if (ops[2].kind != Operand::Kind::LABEL) {
            record_error(ops[2].loc, StringView("expected label"));
            emit_text_word(0, loc); emit_text_word(0, loc);
            return true;
        }

        // First instruction: slt $at, ?, ?
        // Second instruction: beq/bne $at, $0, label  (computed at PC+4)
        Uint32 slt_rs, slt_rt;
        Bool   take_when_nonzero; // true => bne, false => beq
        switch (k) {
        case PseudoKind::BLT: slt_rs = rs; slt_rt = rt; take_when_nonzero = true;  break;
        case PseudoKind::BLE: slt_rs = rt; slt_rt = rs; take_when_nonzero = false; break;
        case PseudoKind::BGT: slt_rs = rt; slt_rt = rs; take_when_nonzero = true;  break;
        case PseudoKind::BGE: slt_rs = rs; slt_rt = rt; take_when_nonzero = false; break;
        default: slt_rs = 0; slt_rt = 0; take_when_nonzero = true; break;
        }

        // slt $at, slt_rs, slt_rt
        emit_text_word(encode_R(FUNCT_SLT, slt_rs, slt_rt, REG_AT, 0), loc);

        // Branch target offset is from PC+4 of the *branch* itself, which
        // is the current text_pc_ after we just emitted slt.
        Uint32 target;
        if (!resolve_label(ops[2].label, ops[2].loc, target)) {
            emit_text_word(0, loc); return true;
        }
        Uint32 imm16;
        if (!branch_offset(text_pc_, target, ops[2].loc, imm16)) {
            emit_text_word(0, loc); return true;
        }
        const auto op = take_when_nonzero ? OP_BNE : OP_BEQ;
        emit_text_word(encode_I(op, REG_AT, REG_ZERO, imm16), loc);
        return true;
    }
    case PseudoKind::_COUNT:
        break;
    }
    return record_error(loc, StringView("internal: unknown pseudo"));
}

Bool Assembler::encode_data_word(Slice<const Operand> ops, Location loc) {
    for (const auto& op : ops) {
        Uint32 v = 0;
        if (op.kind == Operand::Kind::IMM) {
            v = static_cast<Uint32>(static_cast<Uint64>(op.imm));
        } else if (op.kind == Operand::Kind::LABEL) {
            if (!resolve_label(op.label, op.loc, v)) {
                v = 0; // emit zero on failure but keep going
            }
        } else {
            record_error(op.loc, StringView(".word value must be integer or label"));
        }
        if (!emit_data_word(v)) {
            return record_error(loc, StringView("internal: data overflow"));
        }
    }
    return true;
}

Bool Assembler::encode_data_half(Slice<const Operand> ops, Location loc) {
    for (const auto& op : ops) {
        Sint64 v;
        if (!require_imm(op, v)) continue;
        if (v < -32768 || v > 65535) {
            record_error(op.loc, StringView(".half value out of range (16 bits)"));
            v = 0;
        }
        const auto u = static_cast<Uint32>(v) & 0xFFFFu;
        // Big-endian halfword packing (matches vmips32 lh/sh logic).
        if (!emit_data_byte(static_cast<Uint8>((u >> 8) & 0xFFu))) {
            return record_error(loc, StringView("internal: data overflow"));
        }
        if (!emit_data_byte(static_cast<Uint8>( u       & 0xFFu))) {
            return record_error(loc, StringView("internal: data overflow"));
        }
    }
    return true;
}

Bool Assembler::encode_data_byte(Slice<const Operand> ops, Location loc) {
    for (const auto& op : ops) {
        Sint64 v;
        if (!require_imm(op, v)) continue;
        if (v < -128 || v > 255) {
            record_error(op.loc, StringView(".byte value out of range (8 bits)"));
            v = 0;
        }
        if (!emit_data_byte(static_cast<Uint8>(v))) {
            return record_error(loc, StringView("internal: data overflow"));
        }
    }
    return true;
}

Bool Assembler::encode_data_string(StringView text, Bool zero_term, Location loc) {
    Ulen i = 0;
    while (i < text.length()) {
        Uint32 b = 0;
        if (!decode_string_byte(text, i, b, loc)) return true;
        if (!emit_data_byte(static_cast<Uint8>(b & 0xFFu))) {
            return record_error(loc, StringView("internal: data overflow"));
        }
    }
    if (zero_term) {
        if (!emit_data_byte(0)) {
            return record_error(loc, StringView("internal: data overflow"));
        }
    }
    return true;
}

// -- Pass 2: encode ---------------------------------------------------

Bool Assembler::encode() {
    // Allocate the final image with zeros.
    const auto data_words   = (data_size_ + 3) / 4;
    const auto total_words  = (text_size_ / 4) + data_words;
    if (!image_.resize(total_words)) return false;
    for (Ulen i = 0; i < total_words; i++) image_[i] = 0;

    Section section = Section::TEXT;
    text_pc_  = base_;
    data_off_ = 0;

    for (Ulen i = 0; i < stmts_.length(); i++) {
        const auto& s   = stmts_[i];
        const auto  ops = operands_.subrange(s.op_offset, s.op_offset + s.op_count);

        // Mirror pass-1 alignment behavior so we deposit bytes at the
        // same offsets that the layout step recorded.
        Uint32 req = 1;
        switch (s.kind) {
        case Stmt::Kind::INSTR:
        case Stmt::Kind::PSEUDO:
        case Stmt::Kind::DATA_WORD:
            req = 4; break;
        case Stmt::Kind::DATA_HALF:
            req = 2; break;
        default: break;
        }
        if (req > 1) {
            if (section == Section::TEXT) {
                text_pc_ = base_ + align_up(text_pc_ - base_, req);
            } else {
                while ((data_off_ % req) != 0) {
                    if (!emit_data_byte(0)) {
                        return record_error(s.loc, StringView("internal: data overflow"));
                    }
                }
            }
        }

        switch (s.kind) {
        case Stmt::Kind::LABEL_DEF: break;
        case Stmt::Kind::INSTR:
            encode_instr(static_cast<InstrKind>(s.sub), ops, s.loc);
            break;
        case Stmt::Kind::PSEUDO:
            encode_pseudo(static_cast<PseudoKind>(s.sub), ops, s.loc);
            break;
        case Stmt::Kind::SECTION:
            section = (s.sub == static_cast<Uint32>(DirectiveKind::TEXT))
                ? Section::TEXT : Section::DATA;
            break;
        case Stmt::Kind::DATA_WORD:
            encode_data_word(ops, s.loc);
            break;
        case Stmt::Kind::DATA_HALF:
            encode_data_half(ops, s.loc);
            break;
        case Stmt::Kind::DATA_BYTE:
            encode_data_byte(ops, s.loc);
            break;
        case Stmt::Kind::DATA_ASCII:
            encode_data_string(s.text, false, s.loc);
            break;
        case Stmt::Kind::DATA_ASCIIZ:
            encode_data_string(s.text, true, s.loc);
            break;
        case Stmt::Kind::DATA_SPACE: {
            if (ops.length() >= 1 && ops[0].kind == Operand::Kind::IMM) {
                const auto n = ops[0].imm;
                for (Sint64 j = 0; j < n; j++) {
                    if (!emit_data_byte(0)) {
                        return record_error(s.loc, StringView("internal: data overflow"));
                    }
                }
            }
            break;
        }
        case Stmt::Kind::ALIGN: {
            if (ops.length() >= 1 && ops[0].kind == Operand::Kind::IMM) {
                const auto exp = ops[0].imm;
                if (exp >= 0 && exp <= 16) {
                    const auto a = 1u << static_cast<Uint32>(exp);
                    if (section == Section::TEXT) {
                        text_pc_ = base_ + align_up(text_pc_ - base_, a);
                    } else {
                        while ((data_off_ % a) != 0) {
                            emit_data_byte(0);
                        }
                    }
                }
            }
            break;
        }
        case Stmt::Kind::GLOBL: break;
        }
    }
    return true;
}

Bool Assembler::assemble(Slice<const Stmt> stmts, Slice<const Operand> operands) {
    stmts_    = stmts;
    operands_ = operands;
    labels_.clear();
    image_.clear();
    errors_.clear();

    if (!layout()) return false;

    // Even if layout reported errors, we keep going to encode so the
    // user sees as many issues as possible. The image will still be
    // produced (with some words possibly zeroed in error spots).
    return encode();
}
