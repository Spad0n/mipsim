#ifndef VMIPS32_H_
#define VMIPS32_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VMIPS32_ASSERT
void vmips32_trap(const char *message);
#define VMIPS32_ASSERT(condition, message) do { if (!(condition)) vmips32_trap(message); } while(0)
#endif // VMIPS32_ASSERT

#ifndef MEM_SIZE
#define MEM_SIZE 1024
#endif // MEM_SIZE

#ifndef VMIPS32_START
#define VMIPS32_START 0x00400000
#endif // VMIPS32_START

// the user should implement in order to work
void vmips32_syscall(uint32_t v0, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);

void vmips32_load_program(const uint32_t* program, size_t size_in_bytes);

void vmips32_step();

#ifdef VMIPS32_IMPLEMENTATION

#define NUM_REGS 32

typedef enum {
    ZERO =  0,
    AT   =  1,
    V0   =  2,
    V1   =  3,
    A0   =  4,
    A1   =  5,
    A2   =  6,
    A3   =  7,
    T0   =  8,
    T1   =  9,
    T2   = 10,
    T3   = 11,
    T4   = 12,
    T5   = 13,
    T6   = 14,
    T7   = 15,
    S0   = 16,
    S1   = 17,
    S2   = 18,
    S3   = 19,
    S4   = 20,
    S5   = 21,
    S6   = 22,
    S7   = 23,
    T8   = 24,
    T9   = 25,
    K0   = 26,
    K1   = 27,
    GP   = 28,
    SP   = 29,
    FP   = 30,
    RA   = 31,
} REGS;

static struct {
    int32_t regs[NUM_REGS];
    uint32_t PC;
    uint32_t HI;
    uint32_t LO;
    uint32_t memory[MEM_SIZE];
} vm = {
    .PC = VMIPS32_START,
};

static uint32_t* get_memory_ptr(uint32_t addr) {
    if (addr < VMIPS32_START) {
        return NULL;
    }

    uint32_t offset = addr - VMIPS32_START;
    uint32_t index = offset / 4;

    if (index >= MEM_SIZE) {
        VMIPS32_ASSERT(0, "Memory Address Violation (Out of bounds)");
        return NULL;
    }

    return &vm.memory[index];
}

static void null(uint32_t instruction) {
    (void)instruction;
    VMIPS32_ASSERT(0, "Unreachable");
}

/*
  R-type
  shift right logical immediate
  Syntax: sll $d, $t, S
  C code: d = t << S
 */
static void sll(uint32_t instruction) {
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;
    int S  = (instruction >> 6)  & 0x1F;

    vm.regs[rd] = vm.regs[rt] << S;
}

/*
  R-type
  shift left logical immediate
  Syntax: srl $d, $t, S
  C code: d = t >> S (zero extended)
 */
static void srl(uint32_t instruction) {
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;
    int S  = (instruction >> 6)  & 0x1F;

    vm.regs[rd] = (uint32_t)vm.regs[rt] >> S;
}

/*
  R-type
  shift right arithmetic immediate
  Syntax: sra $d, $t, S
  C code: d = t >> S (sign extended)
 */
static void sra(uint32_t instruction) {
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;
    int S  = (instruction >> 6)  & 0x1F;

    vm.regs[rd] = (int32_t)vm.regs[rt] >> S;
}

/*
  R-type
  shift left logical
  Syntax: sllv $d, $t, $s
  C code: d = t << s
 */
static void sllv(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rt] << (vm.regs[rs] & 0x1F);
}

/*
  R-type
  shift right logical
  Syntax: srlv $d, $t, $s
  C code: d = t >> s (zero extended)
 */
static void srlv(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = (uint32_t)vm.regs[rt] >> (vm.regs[rs] & 0x1F);
}

/*
  R-type
  shift right arithmetic
  Syntax: srav $d, $t, $s
  C code: d = t >> s (sign extended)
 */
static void srav(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = (int32_t)vm.regs[rt] >> (vm.regs[rs] & 0x1F);
}

/*
  R-type
  jump register
  Syntax: jr $s
  C code: -
  PC advance: PC = s
 */
static void jr(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;

    vm.PC = vm.regs[rs];
}

/*
  R-type
  jump register and link
  Syntax: jalr $s, $d
  C code: d = PC + 4
  PC advance: PC = s
 */
static void jalr(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.PC + 4;
    vm.PC = vm.regs[rs];
}

/*
  R-type
  move if zero
  Syntax: movz $d, $s, $t
  C code: if (t == 0) d = s
 */
static void movz(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    if (vm.regs[rt] == 0) vm.regs[rd] = vm.regs[rs];
}

/*
  R-type
  move if zero
  Syntax: movn $d, $s, $t
  C code: if (t != 0) d = s
 */
static void movn(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    if (vm.regs[rt] != 0) vm.regs[rd] = vm.regs[rs];
}

/*
  R-type
  system call
  Syntax: syscall
  C code: Trap (handled by loop)
 */
static void _syscall(uint32_t instruction) {
    (void)instruction;
    // Note: Actually handled by main loop decode, but good as fallback
    vmips32_syscall(vm.regs[V0], vm.regs[A0], vm.regs[A1], vm.regs[A2], vm.regs[A3]);
}

/*
  R-type
  system call
  Syntax: syscall
  C code: Trap (handled by loop)
 */
static void _break(uint32_t instruction) {
    (void)instruction;
    VMIPS32_ASSERT(0, "break instruction triggered");
}

/*
  R-type
  move from HI
  Syntax: mfhi $d
  C code: d = HI
 */
static void mfhi(uint32_t instruction) {
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.HI;
}

/*
  R-type
  move to HI
  Syntax: mthi $s
  C code: HI = s
 */
static void mthi(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;

    vm.HI = vm.regs[rs];
}

/*
  R-type
  move from LO
  Syntax: mflo $d
  C code: d = LO
 */
static void mflo(uint32_t instruction) {
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.LO;
}

/*
  R-type
  move to LO
  Syntax: mtlo $s
  C code: LO = s
 */
static void mtlo(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;

    vm.LO = vm.regs[rs];
}

/*
  R-type
  multiply word
  Syntax: mult $s, $t
  C code: (HI, LO) = s * t
 */
static void mult(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    int64_t result = (int64_t)((int32_t)vm.regs[rs]) * (int64_t)((int32_t)vm.regs[rt]);
    vm.LO = (uint32_t)(result & 0xFFFFFFFF);
    vm.HI = (uint32_t)((result >> 32) & 0xFFFFFFFF);
}

/*
  R-type
  multiply unsigned word
  Syntax: multu $s, $t
  C code: (HI, LO) = s * t (unsigned)
 */
static void multu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    uint64_t result = (uint64_t)(uint32_t)vm.regs[rs] * (uint64_t)(uint32_t)vm.regs[rt];
    vm.LO = (uint32_t)(result & 0xFFFFFFFF);
    vm.HI = (uint32_t)((result >> 32) & 0xFFFFFFFF);
}

/*
  R-type
  divide word
  Syntax: div $s, $t
  C code: LO = s / t; HI = s % t
 */
static void _div(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if (vm.regs[rt] != 0) {
        int32_t val1 = (int32_t)vm.regs[rs];
        int32_t val2 = (int32_t)vm.regs[rt];
        vm.LO = val1 / val2;
        vm.HI = val1 % val2;
    }
}

/*
  R-type
  divide unsigned word
  Syntax: divu $s, $t
  C code: LO = s / t; HI = s % t (unsigned)
 */
static void divu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if (vm.regs[rt] != 0) {
        vm.LO = (uint32_t)vm.regs[rs] / (uint32_t)vm.regs[rt];
        vm.HI = (uint32_t)vm.regs[rs] % (uint32_t)vm.regs[rt];
    }
}

/*
  R-type
  add word (with overflow)
  Syntax: add $d, $s, $t
  C code: d = s + t
 */
static void add(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    // NOTE: should trap on overflow
    vm.regs[rd] = vm.regs[rs] + vm.regs[rt];
}

/*
  R-type
  add unsigned word (no overflow)
  Syntax: addu $d, $s, $t
  C code: d = s + t
 */
static void addu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rs] + vm.regs[rt];
}

/*
  R-type
  subtract word (with overflow)
  Syntax: sub $d, $s, $t
  C code: d = s - t
 */
static void sub(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    // NOTE: should trap on overflow
    vm.regs[rd] = vm.regs[rs] - vm.regs[rt];
}

/*
  R-type
  subtract unsigned word (no overflow)
  Syntax: subu $d, $s, $t
  C code: d = s - t
 */
static void subu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rs] - vm.regs[rt];
}

/*
  R-type
  bitwise and
  Syntax: and $d, $s, $t
  C code: d = s & t
 */
static void _and(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rs] & vm.regs[rt];
}

/*
  R-type
  bitwise or
  Syntax: or $d, $s, $t
  C code: d = s | t
 */
static void _or(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rs] | vm.regs[rt];
}

/*
  R-type
  bitwise xor
  Syntax: xor $d, $s, $t
  C code: d = s ^ t
 */
static void _xor(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = vm.regs[rs] ^ vm.regs[rt];
}

/*
  R-type
  bitwise nor
  Syntax: nor $d, $s, $t
  C code: d = ~(s | t)
 */
static void nor(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = ~ (vm.regs[rs] | vm.regs[rt]);
}

/*
  R-type
  set on less than
  Syntax: slt $d, $s, $t
  C code: d = (s < t) ? 1 : 0
 */
static void slt(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = (int32_t)vm.regs[rs] < (int32_t)vm.regs[rt] ? 1 : 0;
}

/*
  R-type
  set on less than unsigned
  Syntax: sltu $d, $s, $t
  C code: d = (s < t) ? 1 : 0 (unsigned)
 */
static void sltu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    vm.regs[rd] = (uint32_t)vm.regs[rs] < (uint32_t)vm.regs[rt] ? 1 : 0;
}

/*
  R-type
  trap if greater or equal
  Syntax: tge $s, $t
  C code: if (s >= t) Trap()
 */
static void tge(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if ((int32_t)vm.regs[rs] >= (int32_t)vm.regs[rt]) {
        VMIPS32_ASSERT(0, "TGE Trap");
    }
}

/*
  R-type
  trap if greater or equal unsigned
  Syntax: tgeu $s, $t
  C code: if (s >= t) Trap() (unsigned)
 */
static void tgeu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if ((uint32_t)vm.regs[rs] >= (uint32_t)vm.regs[rt]) {
        VMIPS32_ASSERT(0, "Trap: TGEU");
    }
}

/*
  R-type
  trap if less than
  Syntax: tlt $s, $t
  C code: if (s < t) Trap()
 */
static void tlt(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if ((int32_t)vm.regs[rs] < (int32_t)vm.regs[rt]) {
        VMIPS32_ASSERT(0, "TLT Trap");
    }
}

/*
  R-type
  trap if less than unsigned
  Syntax: tltu $s, $t
  C code: if (s < t) Trap() (unsigned)
 */
static void tltu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if ((uint32_t)vm.regs[rs] < (uint32_t)vm.regs[rt]) {
        VMIPS32_ASSERT(0, "TLTU Trap");
    }
}

/*
  R-type
  trap if equal
  Syntax: teq $s, $t
  C code: if (s == t) Trap()
 */
static void teq(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if (vm.regs[rs] == vm.regs[rt]) {
        VMIPS32_ASSERT(0, "Trap: TEQ");
    }
}

/*
  R-type
  trap if not equal
  Syntax: tne $s, $t
  C code: if (s != t) Trap()
 */
static void tne(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;

    if (vm.regs[rs] != vm.regs[rt]) {
        VMIPS32_ASSERT(0, "Trap: TNE");
    }
}

static void (*rtables[64])(uint32_t) = {
    sll, /* movci */ null, srl, sra, sllv, null, srlv, srav,
    jr, jalr, movz, movn, _syscall, _break, null, /* sync */ null,
    mfhi, mthi, mflo, mtlo, null, null, null, null,
    mult, multu, _div, divu, null, null, null, null,
    add, addu, sub, subu, _and, _or, _xor, nor,
    null, null, slt, sltu, null, null, null, null,
    tge, tgeu, tlt, tltu, teq, null, tne, null,
    null, null, null, null, null, null, null, null,
};

/*
  J-type
  jump
  Syntax: j target
  C code: PC = target
 */
static void j(uint32_t instruction) {
    int address = instruction & 0x3FFFFFF;

    vm.PC = (vm.PC & 0xf0000000) | (address << 2);
}

/*
  J-type
  jump and link
  Syntax: jal target
  C code: ra = PC + 8; PC = target
 */
static void jal(uint32_t instruction) {
    int address = instruction & 0x3FFFFFF;

    vm.regs[RA] = vm.PC + 4;
    vm.PC = (vm.PC & 0xf0000000) | (address << 2);
}

/*
  I-type
  branch on equal
  Syntax: beq $s, $t, offset
  C code: if (s == t) PC += offset
 */
static void beq(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (vm.regs[rs] == vm.regs[rt]) {
        vm.PC += 4 + (imm << 2);
    }
}

/*
  I-type
  branch on not equal
  Syntax: bne $s, $t, offset
  C code: if (s != t) PC += offset
 */
static void bne(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (vm.regs[rs] != vm.regs[rt]) {
        vm.PC += 4 + (imm << 2);
    }
}

/*
  I-type
  branch on less than or equal to zero
  Syntax: blez $s, offset
  C code: if (s <= 0) PC += offset
 */
static void blez(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if ((int32_t)vm.regs[rs] <= 0) {
        vm.PC += 4 + (imm << 2);
    }
}

/*
  I-type
  branch on greater than zero
  Syntax: bgtz $s, offset
  C code: if (s > 0) PC += offset
 */
static void bgtz(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if ((int32_t)vm.regs[rs] > 0) {
        vm.PC += 4 + (imm << 2);
    }
}

/*
  I-type
  add immediate (with overflow)
  Syntax: addi $t, $s, imm
  C code: t = s + imm
 */
static void addi(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = vm.regs[rs] + imm;
}

/*
  I-type
  add immediate unsigned (no overflow)
  Syntax: addiu $t, $s, imm
  C code: t = s + imm
 */
static void addiu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = vm.regs[rs] + imm;
}

/*
  I-type
  set on less than immediate
  Syntax: slti $t, $s, imm
  C code: t = (s < imm) ? 1 : 0
 */
static void slti(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = (int32_t)vm.regs[rs] < (int32_t)imm ? 1 : 0;
}

/*
  I-type
  set on less than immediate unsigned
  Syntax: sltiu $t, $s, imm
  C code: t = (s < imm) ? 1 : 0 (unsigned)
 */
static void sltiu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    int32_t sign_extended_imm = imm;

    vm.regs[rt] = (uint32_t)vm.regs[rs] < (uint32_t)sign_extended_imm ? 1 : 0;
}

/*
  I-type
  bitwise and immediate
  Syntax: andi $t, $s, imm
  C code: t = s & imm
 */
static void andi(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    uint16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = vm.regs[rs] & imm;
}

/*
  I-type
  bitwise or immediate
  Syntax: ori $t, $s, imm
  C code: t = s | imm
 */
static void ori(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    uint16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = vm.regs[rs] | imm;
}

/*
  I-type
  bitwise xor immediate
  Syntax: xori $t, $s, imm
  C code: t = s ^ imm
 */
static void xori(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    uint16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = vm.regs[rs] ^ imm;
}

/*
  I-type
  load upper immediate
  Syntax: lui $t, imm
  C code: t = imm << 16
 */
static void lui(uint32_t instruction) {
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    vm.regs[rt] = imm << 16;
}

/*
  I-type
  branch on equal likely
  Syntax: beql $s, $t, offset
  C code: if (s == t) PC += offset else cancel_delay_slot()
 */
static void beql(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (vm.regs[rs] == vm.regs[rt]) {
        vm.PC += 4 + (imm << 2);
    } else {
        // cancel the delay slot by jumping onto it
        vm.PC += 8;
    }
}

/*
  I-type
  branch on not equal likely
  Syntax: bnel $s, $t, offset
  C code: if (s != t) PC += offset else cancel_delay_slot()
 */
static void bnel(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if (vm.regs[rs] != vm.regs[rt]) {
        vm.PC += 4 + (imm << 2);
    } else {
        // cancel the delay slot by jumping onto it
        vm.PC += 8;
    }
}

/*
  I-type
  branch on less than or equal to zero likely
  Syntax: blezl $s, offset
  C code: if (s <= 0) PC += offset else cancel_delay_slot()
 */
static void blezl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if ((int32_t)vm.regs[rs] <= 0) {
        vm.PC += 4 + (imm << 2);
    } else {
        // cancel the delay slot by jumping onto it
        vm.PC += 8;
    }
}

/*
  I-type
  branch on greater than zero likely
  Syntax: bgtzl $s, offset
  C code: if (s > 0) PC += offset else cancel_delay_slot()
 */
static void bgtzl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    if ((int32_t)vm.regs[rs] > 0) {
        vm.PC += 4 + (imm << 2);
    } else {
        // cancel the delay slot by jumping onto it
        vm.PC += 8;
    }
}

/*
  I-type
  load byte
  Syntax: lb $t, offset($s)
  C code: t = (int8)memory[s + offset]
 */
static void lb(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;

    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);

    if (cell) {
        int shift = (3 - (addr & 3)) * 8;
        int32_t byte = (*cell >> shift) & 0xFF;
        
        vm.regs[rt] = (int32_t)(int8_t)byte;
    }
}

/*
  I-type
  load halfword
  Syntax: lh $t, offset($s)
  C code: t = (int16)memory[s + offset]
 */
static void lh(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    VMIPS32_ASSERT((addr & 1) == 0, "Unaligned memory access (lh)");
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int shift = (2 - (addr & 2)) * 8;
        int32_t half = (*cell >> shift) & 0xFFFF;
        vm.regs[rt] = (int32_t)(int16_t)half;
    }
}

/*
  I-type
  load word left
  Syntax: lwl $t, offset($s)
  C code: Load upper bytes of word
 */
static void lwl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr); 
    if (cell) {
        int byte_offset = addr & 3;
        uint32_t reg_mask = (1 << (byte_offset * 8)) - 1; 
        vm.regs[rt] = (vm.regs[rt] & reg_mask) | (*cell << (byte_offset * 8));
    }
}

/*
  I-type
  load word
  Syntax: lw $t, offset($s)
  C code: t = memory[s + offset]
 */
static void lw(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    VMIPS32_ASSERT(addr % 4 == 0, "Unaligned memory access (lw)");
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) vm.regs[rt] = *cell;
}

/*
  I-type
  load byte unsigned
  Syntax: lbu $t, offset($s)
  C code: t = (uint8)memory[s + offset]
 */
static void lbu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int shift = (3 - (addr & 3)) * 8;
        vm.regs[rt] = (*cell >> shift) & 0xFF;
    }
}

/*
  I-type
  load halfword unsigned
  Syntax: lhu $t, offset($s)
  C code: t = (uint16)memory[s + offset]
 */
static void lhu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    VMIPS32_ASSERT((addr & 1) == 0, "Unaligned memory access (lhu)");
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int shift = (2 - (addr & 2)) * 8;
        vm.regs[rt] = (*cell >> shift) & 0xFFFF;
    }
}

/*
  I-type
  load word right
  Syntax: lwr $t, offset($s)
  C code: Load lower bytes of word
 */
static void lwr(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int byte_offset = addr & 3;
        uint32_t shift = (3 - byte_offset) * 8;
        uint32_t val = *cell >> shift;
        uint32_t keep_mask = ~(0xFFFFFFFF >> shift);
        if (shift == 32) keep_mask = 0;
        vm.regs[rt] = (vm.regs[rt] & keep_mask) | val;
    }
}

/*
  I-type
  load word unsigned (MIPS64 - same as LW in 32bit)
  Syntax: lwu $t, offset($s)
 */
static void lwu(uint32_t instruction) {
    lw(instruction);
}

/*
  I-type
  store byte
  Syntax: sb $t, offset($s)
  C code: memory[s + offset] = (int8)t
 */
static void sb(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int shift = (3 - (addr & 3)) * 8;
        uint32_t mask = ~(0xFF << shift);
        *cell = (*cell & mask) | ((vm.regs[rt] & 0xFF) << shift);
    }
}

/*
  I-type
  store halfword
  Syntax: sh $t, offset($s)
  C code: memory[s + offset] = (int16)t
 */
static void sh(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    VMIPS32_ASSERT((addr & 1) == 0, "Unaligned memory access (sh)");
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int shift = (2 - (addr & 2)) * 8;
        uint32_t mask = ~(0xFFFF << shift);
        *cell = (*cell & mask) | ((vm.regs[rt] & 0xFFFF) << shift);
    }
}

/*
  I-type
  store word left
  Syntax: swl $t, offset($s)
  C code: Store upper bytes of register to unaligned memory
 */
static void swl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int byte_offset = addr & 3;
        uint32_t reg_val = vm.regs[rt];
        // Bitwise implementation approximation
        //uint32_t shift = (byte_offset) * 8;
        // This logic depends heavily on endianness interpretation
        // Simplified:
        uint32_t mask = 0xFFFFFFFF >> ((3 - byte_offset) * 8);
        *cell = (*cell & mask) | (reg_val >> ((3-byte_offset) * 8));
    }
}

/*
  I-type
  store word
  Syntax: sw $t, offset($s)
  C code: memory[s + offset] = t
 */
static void sw(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    VMIPS32_ASSERT(addr % 4 == 0, "Unaligned memory access (sw)");
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) *cell = vm.regs[rt];
}

/*
  I-type
  store word right
  Syntax: swr $t, offset($s)
  C code: Store lower bytes of register to unaligned memory
 */
static void swr(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    uint32_t addr = vm.regs[rs] + (int32_t)imm;
    uint32_t* cell = get_memory_ptr(addr);
    if (cell) {
        int byte_offset = addr & 3;
        uint32_t mask = 0xFFFFFFFF << ((3 - byte_offset) * 8);
        *cell = (*cell & ~mask) | (vm.regs[rt] << ((3 - byte_offset) * 8));
    }
}

static void (*itables[64])(uint32_t) = {
    null,  null,    j,   jal,  beq,  bne,  blez,  bgtz,
    addi, addiu, slti, sltiu, andi,  ori,  xori,   lui,
    null,  null, null,  null, beql, bnel, blezl, bgtzl,
    null,  null, null,  null, null, null, null, null,
    lb, lh, lwl,  lw, lbu, lhu, lwr, lwu,
    sb, sh, swl,  sw, null, null, swr, null,
    null, null, null,  null, null, null, null, null,
    null /* sc */, null, null,  null, null, null, null, null /* sd */,
};

/*
  REGIMM
  branch on less than zero
  Syntax: bltz $s, offset
  C code: if (s < 0) PC += offset
 */
static void bltz(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] < 0) vm.PC += 4 + (imm << 2);
}

/*
  REGIMM
  branch on greater than or equal to zero
  Syntax: bgez $s, offset
  C code: if (s >= 0) PC += offset
 */
static void bgez(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] >= 0) vm.PC += 4 + (imm << 2);
}

/*
  REGIMM
  branch on less than zero likely
  Syntax: bltzl $s, offset
  C code: if (s < 0) execute_delay_slot(); else skip_delay_slot();
 */
static void bltzl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] < 0) {
        vm.PC += 4 + (imm << 2);
    } else {
        vm.PC += 8;
    }
}

/*
  REGIMM
  branch on greater than or equal to zero likely
  Syntax: bgezl $s, offset
  C code: if (s >= 0) execute_delay_slot(); else skip_delay_slot();
 */
static void bgezl(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] >= 0) {
        vm.PC += 4 + (imm << 2);
    } else {
        vm.PC += 8;
    }
}

/*
  REGIMM
  trap if greater or equal immediate
  Syntax: tgei $s, imm
  C code: if (s >= imm) Trap()
 */
static void tgei(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] >= (int32_t)imm) {
        VMIPS32_ASSERT(0, "Trap: TGEI");
    }
}

/*
  REGIMM
  trap if less than immediate
  Syntax: tlti $s, imm
  C code: if (s < imm) Trap()
 */
static void tlti(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] < (int32_t)imm) {
        VMIPS32_ASSERT(0, "Trap: TLTI");
    }
}

/*
  REGIMM
  trap if less than immediate unsigned
  Syntax: tltiu $s, imm
  C code: if ((uint32)s < (uint32)imm) Trap()
 */
static void tltiu(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((uint32_t)vm.regs[rs] < (uint32_t)(int32_t)imm) {
        VMIPS32_ASSERT(0, "Trap: TLTIU");
    }
}

/*
  REGIMM
  trap if equal immediate
  Syntax: teqi $s, imm
  C code: if (s == imm) Trap()
 */
static void teqi(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] == (int32_t)imm) {
        VMIPS32_ASSERT(0, "Trap: TEQI");
    }
}

/*
  REGIMM
  trap if not equal immediate
  Syntax: tnei $s, imm
  C code: if (s != imm) Trap()
 */
static void tnei(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] != (int32_t)imm) {
        VMIPS32_ASSERT(0, "Trap: TNEI");
    }
}

/*
  REGIMM
  branch on less than zero and link
  Syntax: bltzal $s, offset
  C code: if (s < 0) { ra = PC + 8; PC += offset; }
 */
static void bltzal(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] < 0) {
        vm.regs[RA] = vm.PC + 8;
        vm.PC += 4 + (imm << 2);
    }
}

/*
  REGIMM
  branch on greater than or equal to zero and link
  Syntax: bgezal $s, offset
  C code: if (s >= 0) { ra = PC + 8; PC += offset; }
 */
static void bgezal(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] >= 0) {
        vm.regs[RA] = vm.PC + 8;
        vm.PC += 4 + (imm << 2);
    }
}

/*
  REGIMM
  branch on less than zero and link likely
  Syntax: bltzall $s, offset
  C code: if (s < 0) { ra = PC + 8; execute_delay_slot(); } else skip_delay_slot();
 */
static void bltzall(uint32_t instruction) {
    int rs = (instruction >> 21) & 0x1F;
    int16_t imm = instruction & 0xFFFF;
    if ((int32_t)vm.regs[rs] < 0) {
        vm.regs[RA] = vm.PC + 8;
        vm.PC += 4 + (imm << 2);
    } else {
        vm.PC += 8;
    }
}

static void (*ritables[32])(uint32_t) = {
    bltz, bgez, bltzl, bgezl, sllv, null, null, null,
    tgei, jalr, tlti, tltiu, teqi, null, tnei, null,
    bltzal, bgezal, bltzall, null, null, null, null,
    null, null, null, null, null, null, null, null,
};

void vmips32_load_program(const uint32_t *program, size_t size_in_bytes) {
    size_t count = size_in_bytes / 4;
    VMIPS32_ASSERT(count <= MEM_SIZE, "Program too large for memory");

    for (size_t i = 0; i < count; i++) {
        vm.memory[i] = program[i];
    }

    vm.PC = VMIPS32_START;

    for (int i = 0; i < NUM_REGS; i++) vm.regs[i] = 0;
}

void vmips32_step() {
    uint32_t *pc_ptr = get_memory_ptr(vm.PC);
    VMIPS32_ASSERT(pc_ptr != NULL, "PC execution out of bounds");
    uint32_t instruction = *pc_ptr;
    //printf("PC: 0x%08X | Instr: 0x%08X\n", vm.PC, instruction);

    uint32_t current_pc = vm.PC;

    int opcode = (instruction >> 26) & 0x3F;
    if (opcode == 0x00) {
        int funct = instruction & 0x3F;
        if (funct == 0x0C) {
            vmips32_syscall(vm.regs[V0], vm.regs[A0], vm.regs[A1], vm.regs[A2], vm.regs[A3]);
        } else if (funct == 0x0D) {
            // break
        } else {
            if (rtables[funct]) rtables[funct](instruction);
        }
    } else if (opcode == 0x01) {
        int regimm = (instruction >> 16) & 0x1F;
        if (ritables[regimm]) ritables[regimm](instruction);
    } else {
        if (itables[opcode]) itables[opcode](instruction);
    }

    if (vm.PC == current_pc) {
        vm.PC += 4;
    }

    vm.regs[ZERO] = 0;
}

#endif // VMIPS32_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // VMIPS32_H_
