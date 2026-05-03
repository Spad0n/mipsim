#ifndef TOK_KIND
#define TOK_KIND(...)
#endif

#ifndef TOK_REG
#define TOK_REG(...)
#endif

#ifndef TOK_INSTR
#define TOK_INSTR(...)
#endif

#ifndef TOK_PSEUDO
#define TOK_PSEUDO(...)
#endif

#ifndef TOK_DIRECTIVE
#define TOK_DIRECTIVE(...)
#endif

//       ENUM        NAME
TOK_KIND(ENDOF,      "eof")
TOK_KIND(NEWLINE,    "newline")        // Statement separator.
TOK_KIND(INVALID,    "invalid")
TOK_KIND(IDENT,      "identifier")     // Label definitions or label references.
TOK_KIND(REGISTER,   "register")       // $name or $N. value = 0..31.
TOK_KIND(INSTR,      "instruction")    // Recognized real mnemonic. sub = InstrKind.
TOK_KIND(PSEUDO,     "pseudo-instr")   // Recognized pseudo. sub = PseudoKind.
TOK_KIND(DIRECTIVE,  "directive")      // .name. sub = DirectiveKind.
TOK_KIND(INTEGER,    "integer")        // Unsigned literal in int_value.
TOK_KIND(STRING,     "string")         // text holds raw content (still escaped).
TOK_KIND(CHAR,       "character")      // value (decoded codepoint) in int_value.
TOK_KIND(COMMA,      ",")
TOK_KIND(LPAREN,     "(")
TOK_KIND(RPAREN,     ")")
TOK_KIND(COLON,      ":")
TOK_KIND(PLUS,       "+")
TOK_KIND(MINUS,      "-")

//      MNEMONIC   NUMBER
TOK_REG("0",       0)
TOK_REG("zero",    0)
TOK_REG("1",       1)
TOK_REG("at",      1)
TOK_REG("2",       2)
TOK_REG("v0",      2)
TOK_REG("3",       3)
TOK_REG("v1",      3)
TOK_REG("4",       4)
TOK_REG("a0",      4)
TOK_REG("5",       5)
TOK_REG("a1",      5)
TOK_REG("6",       6)
TOK_REG("a2",      6)
TOK_REG("7",       7)
TOK_REG("a3",      7)
TOK_REG("8",       8)
TOK_REG("t0",      8)
TOK_REG("9",       9)
TOK_REG("t1",      9)
TOK_REG("10",      10)
TOK_REG("t2",      10)
TOK_REG("11",      11)
TOK_REG("t3",      11)
TOK_REG("12",      12)
TOK_REG("t4",      12)
TOK_REG("13",      13)
TOK_REG("t5",      13)
TOK_REG("14",      14)
TOK_REG("t6",      14)
TOK_REG("15",      15)
TOK_REG("t7",      15)
TOK_REG("16",      16)
TOK_REG("s0",      16)
TOK_REG("17",      17)
TOK_REG("s1",      17)
TOK_REG("18",      18)
TOK_REG("s2",      18)
TOK_REG("19",      19)
TOK_REG("s3",      19)
TOK_REG("20",      20)
TOK_REG("s4",      20)
TOK_REG("21",      21)
TOK_REG("s5",      21)
TOK_REG("22",      22)
TOK_REG("s6",      22)
TOK_REG("23",      23)
TOK_REG("s7",      23)
TOK_REG("24",      24)
TOK_REG("t8",      24)
TOK_REG("25",      25)
TOK_REG("t9",      25)
TOK_REG("26",      26)
TOK_REG("k0",      26)
TOK_REG("27",      27)
TOK_REG("k1",      27)
TOK_REG("28",      28)
TOK_REG("gp",      28)
TOK_REG("29",      29)
TOK_REG("sp",      29)
TOK_REG("30",      30)
TOK_REG("fp",      30)
TOK_REG("31",      31)
TOK_REG("ra",      31)

// ---- R-type (major opcode = 0, code = funct) --------------------------------
//        ENUM      MNEMONIC    FORMAT       CODE
TOK_INSTR(SLL,      "sll",      R_RD_RT_SA,  0x00)
TOK_INSTR(SRL,      "srl",      R_RD_RT_SA,  0x02)
TOK_INSTR(SRA,      "sra",      R_RD_RT_SA,  0x03)
TOK_INSTR(SLLV,     "sllv",     R_RD_RT_RS,  0x04)
TOK_INSTR(SRLV,     "srlv",     R_RD_RT_RS,  0x06)
TOK_INSTR(SRAV,     "srav",     R_RD_RT_RS,  0x07)
TOK_INSTR(JR,       "jr",       R_RS,        0x08)
TOK_INSTR(JALR,     "jalr",     R_RD_RS,     0x09)
TOK_INSTR(MOVZ,     "movz",     R_RD_RS_RT,  0x0A)
TOK_INSTR(MOVN,     "movn",     R_RD_RS_RT,  0x0B)
TOK_INSTR(SYSCALL,  "syscall",  R_NONE,      0x0C)
TOK_INSTR(BREAK,    "break",    R_NONE,      0x0D)
TOK_INSTR(MFHI,     "mfhi",     R_RD,        0x10)
TOK_INSTR(MTHI,     "mthi",     R_RS,        0x11)
TOK_INSTR(MFLO,     "mflo",     R_RD,        0x12)
TOK_INSTR(MTLO,     "mtlo",     R_RS,        0x13)
TOK_INSTR(MULT,     "mult",     R_RS_RT,     0x18)
TOK_INSTR(MULTU,    "multu",    R_RS_RT,     0x19)
TOK_INSTR(DIV,      "div",      R_RS_RT,     0x1A)
TOK_INSTR(DIVU,     "divu",     R_RS_RT,     0x1B)
TOK_INSTR(ADD,      "add",      R_RD_RS_RT,  0x20)
TOK_INSTR(ADDU,     "addu",     R_RD_RS_RT,  0x21)
TOK_INSTR(SUB,      "sub",      R_RD_RS_RT,  0x22)
TOK_INSTR(SUBU,     "subu",     R_RD_RS_RT,  0x23)
TOK_INSTR(AND,      "and",      R_RD_RS_RT,  0x24)
TOK_INSTR(OR,       "or",       R_RD_RS_RT,  0x25)
TOK_INSTR(XOR,      "xor",      R_RD_RS_RT,  0x26)
TOK_INSTR(NOR,      "nor",      R_RD_RS_RT,  0x27)
TOK_INSTR(SLT,      "slt",      R_RD_RS_RT,  0x2A)
TOK_INSTR(SLTU,     "sltu",     R_RD_RS_RT,  0x2B)
TOK_INSTR(TGE,      "tge",      R_RS_RT,     0x30)
TOK_INSTR(TGEU,     "tgeu",     R_RS_RT,     0x31)
TOK_INSTR(TLT,      "tlt",      R_RS_RT,     0x32)
TOK_INSTR(TLTU,     "tltu",     R_RS_RT,     0x33)
TOK_INSTR(TEQ,      "teq",      R_RS_RT,     0x34)
TOK_INSTR(TNE,      "tne",      R_RS_RT,     0x36)

// ---- J-type (code = opcode) -------------------------------------------------
TOK_INSTR(J,        "j",        J_LBL,       0x02)
TOK_INSTR(JAL,      "jal",      J_LBL,       0x03)

// ---- I-type (code = opcode) -------------------------------------------------
TOK_INSTR(BEQ,      "beq",      I_RS_RT_LBL, 0x04)
TOK_INSTR(BNE,      "bne",      I_RS_RT_LBL, 0x05)
TOK_INSTR(BLEZ,     "blez",     I_RS_LBL,    0x06)
TOK_INSTR(BGTZ,     "bgtz",     I_RS_LBL,    0x07)
TOK_INSTR(ADDI,     "addi",     I_RT_RS_IMM, 0x08)
TOK_INSTR(ADDIU,    "addiu",    I_RT_RS_IMM, 0x09)
TOK_INSTR(SLTI,     "slti",     I_RT_RS_IMM, 0x0A)
TOK_INSTR(SLTIU,    "sltiu",    I_RT_RS_IMM, 0x0B)
TOK_INSTR(ANDI,     "andi",     I_RT_RS_IMM, 0x0C)
TOK_INSTR(ORI,      "ori",      I_RT_RS_IMM, 0x0D)
TOK_INSTR(XORI,     "xori",     I_RT_RS_IMM, 0x0E)
TOK_INSTR(LUI,      "lui",      I_RT_IMM,    0x0F)
TOK_INSTR(BEQL,     "beql",     I_RS_RT_LBL, 0x14)
TOK_INSTR(BNEL,     "bnel",     I_RS_RT_LBL, 0x15)
TOK_INSTR(BLEZL,    "blezl",    I_RS_LBL,    0x16)
TOK_INSTR(BGTZL,    "bgtzl",    I_RS_LBL,    0x17)
TOK_INSTR(LB,       "lb",       I_RT_MEM,    0x20)
TOK_INSTR(LH,       "lh",       I_RT_MEM,    0x21)
TOK_INSTR(LWL,      "lwl",      I_RT_MEM,    0x22)
TOK_INSTR(LW,       "lw",       I_RT_MEM,    0x23)
TOK_INSTR(LBU,      "lbu",      I_RT_MEM,    0x24)
TOK_INSTR(LHU,      "lhu",      I_RT_MEM,    0x25)
TOK_INSTR(LWR,      "lwr",      I_RT_MEM,    0x26)
TOK_INSTR(LWU,      "lwu",      I_RT_MEM,    0x27)
TOK_INSTR(SB,       "sb",       I_RT_MEM,    0x28)
TOK_INSTR(SH,       "sh",       I_RT_MEM,    0x29)
TOK_INSTR(SWL,      "swl",      I_RT_MEM,    0x2A)
TOK_INSTR(SW,       "sw",       I_RT_MEM,    0x2B)
TOK_INSTR(SWR,      "swr",      I_RT_MEM,    0x2E)

// ---- REGIMM (major opcode = 1, code = rt sub-op) ----------------------------
TOK_INSTR(BLTZ,     "bltz",     REGIMM_RS_LBL, 0x00)
TOK_INSTR(BGEZ,     "bgez",     REGIMM_RS_LBL, 0x01)
TOK_INSTR(BLTZL,    "bltzl",    REGIMM_RS_LBL, 0x02)
TOK_INSTR(BGEZL,    "bgezl",    REGIMM_RS_LBL, 0x03)
TOK_INSTR(TGEI,     "tgei",     REGIMM_RS_IMM, 0x08)
TOK_INSTR(TLTI,     "tlti",     REGIMM_RS_IMM, 0x0A)
TOK_INSTR(TLTIU,    "tltiu",    REGIMM_RS_IMM, 0x0B)
TOK_INSTR(TEQI,     "teqi",     REGIMM_RS_IMM, 0x0C)
TOK_INSTR(TNEI,     "tnei",     REGIMM_RS_IMM, 0x0E)
TOK_INSTR(BLTZAL,   "bltzal",   REGIMM_RS_LBL, 0x10)
TOK_INSTR(BGEZAL,   "bgezal",   REGIMM_RS_LBL, 0x11)
TOK_INSTR(BLTZALL,  "bltzall",  REGIMM_RS_LBL, 0x12)

//         ENUM   MNEMONIC
TOK_PSEUDO(NOP,   "nop")     // sll $0, $0, 0
TOK_PSEUDO(MOVE,  "move")    // addu rd, rs, $0
TOK_PSEUDO(LI,    "li")      // addiu / lui+ori depending on imm
TOK_PSEUDO(LA,    "la")      // lui + ori with label hi/lo
TOK_PSEUDO(B,     "b")       // beq $0, $0, label
TOK_PSEUDO(BAL,   "bal")     // bgezal $0, label
TOK_PSEUDO(BEQZ,  "beqz")    // beq rs, $0, label
TOK_PSEUDO(BNEZ,  "bnez")    // bne rs, $0, label
TOK_PSEUDO(NEG,   "neg")     // sub rd, $0, rs
TOK_PSEUDO(NEGU,  "negu")    // subu rd, $0, rs
TOK_PSEUDO(NOT,   "not")     // nor rd, rs, $0
TOK_PSEUDO(BLT,   "blt")     // slt $at, rs, rt; bne $at, $0, label
TOK_PSEUDO(BLE,   "ble")     // slt $at, rt, rs; beq $at, $0, label
TOK_PSEUDO(BGT,   "bgt")     // slt $at, rt, rs; bne $at, $0, label
TOK_PSEUDO(BGE,   "bge")     // slt $at, rs, rt; beq $at, $0, label

//            ENUM     MNEMONIC
TOK_DIRECTIVE(TEXT,    ".text")
TOK_DIRECTIVE(DATA,    ".data")
TOK_DIRECTIVE(WORD,    ".word")
TOK_DIRECTIVE(HALF,    ".half")
TOK_DIRECTIVE(BYTE,    ".byte")
TOK_DIRECTIVE(ASCII,   ".ascii")
TOK_DIRECTIVE(ASCIIZ,  ".asciiz")
TOK_DIRECTIVE(SPACE,   ".space")
TOK_DIRECTIVE(ALIGN,   ".align")
TOK_DIRECTIVE(GLOBL,   ".globl")

#undef TOK_KIND
#undef TOK_REG
#undef TOK_INSTR
#undef TOK_PSEUDO
#undef TOK_DIRECTIVE
