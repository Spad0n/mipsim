#include "token.hpp"

static const StringView kind_names_[] = {
#define TOK_KIND(e, name) name,
#include "tokens.inl"
};

StringView token_kind_name(TokenKind k) {
    const auto i = static_cast<Uint32>(k);
    if (i >= sizeof(kind_names_) / sizeof(*kind_names_)) {
        return StringView("?");
    }
    return kind_names_[i];
}

struct InstrInfo {
    StringView  mnemonic;
    InstrFormat format;
    Uint32      code;
};

static const InstrInfo instr_info_[] = {
#define TOK_INSTR(e, mnem, fmt, code) { StringView(mnem), InstrFormat::fmt, Uint32(code) },
#include "tokens.inl"
};

StringView instr_mnemonic(InstrKind k) {
    return instr_info_[static_cast<Uint32>(k)].mnemonic;
}

InstrFormat instr_format(InstrKind k) {
    return instr_info_[static_cast<Uint32>(k)].format;
}

Uint32 instr_code(InstrKind k) {
    return instr_info_[static_cast<Uint32>(k)].code;
}

Bool lookup_instr(StringView name, InstrKind& out) {
    Uint32 i = 0;
    for (const auto& e : instr_info_) {
        if (e.mnemonic == name) {
            out = static_cast<InstrKind>(i);
            return true;
        }
        i++;
    }
    return false;
}

static const StringView pseudo_names_[] = {
#define TOK_PSEUDO(e, mnem) StringView(mnem),
#include "tokens.inl"
};

StringView pseudo_mnemonic(PseudoKind k) {
    return pseudo_names_[static_cast<Uint32>(k)];
}

Bool lookup_pseudo(StringView name, PseudoKind& out) {
    Uint32 i = 0;
    for (const auto& e : pseudo_names_) {
        if (e == name) {
            out = static_cast<PseudoKind>(i);
            return true;
        }
        i++;
    }
    return false;
}

static const StringView directive_names_[] = {
#define TOK_DIRECTIVE(e, mnem) StringView(mnem),
#include "tokens.inl"
};

StringView directive_mnemonic(DirectiveKind k) {
    return directive_names_[static_cast<Uint32>(k)];
}

Bool lookup_directive(StringView name, DirectiveKind& out) {
    Uint32 i = 0;
    for (const auto& e : directive_names_) {
        if (e == name) {
            out = static_cast<DirectiveKind>(i);
            return true;
        }
        i++;
    }
    return false;
}

struct RegEntry {
    StringView name;
    Uint32     number;
};

static const RegEntry reg_table_[] = {
#define TOK_REG(name, number) { StringView(name), Uint32(number) },
#include "tokens.inl"
};

Bool lookup_register(StringView name, Uint32& out) {
    for (const auto& e : reg_table_) {
        if (e.name == name) {
            out = e.number;
            return true;
        }
    }
    return false;
}
