#ifndef VM_H_
#define VM_H_

#include <stdint.h>
#include <stddef.h>

#define NUM_REGS 32
#define MEM_SIZE 1024

typedef struct {
    uint32_t regs[NUM_REGS];
    uint32_t PC;
    uint32_t memory[MEM_SIZE];
} MIPS_VM;

enum REG_NAME {
    REG_ZERO = 0,
    REG_AT,

    REG_V0,
    REG_V1,

    REG_A0,
    REG_A1,
    REG_A2,
    REG_A3,

    REG_T0,
    REG_T1,
    REG_T2,
    REG_T3,
    REG_T4,
    REG_T5,
    REG_T6,
    REG_T7,

    REG_S0,
    REG_S1,
    REG_S2,
    REG_S3,
    REG_S4,
    REG_S5,
    REG_S6,
    REG_S7,

    REG_T8,
    REG_T9,

    REG_K0,
    REG_K1,

    REG_GP,
    REG_SP,

    REG_S8,
    REG_RA,
};

enum OP_CODE {
    OP_ADDI           = 0x8,
    OP_ADDIU          = 0x9,

    OP_ADD            = 0x20,
    OP_ADDU           = 0x21,
    OP_SUB            = 0x22,
    OP_SUBU           = 0x23,

    OP_JUMP           = 0x2,
    OP_JUMP_AND_LINK  = 0x3,

    // Special instructions
    SYSCALL           = 0xC,
    BREAK             = 0xD,
};

const char* vm_get_opcode_name(enum OP_CODE opcode);

const char* vm_get_reg_name(enum REG_NAME reg);

void vm_execute_many(MIPS_VM *vm, uint32_t *instruction, size_t num_instruction);

void vm_execute_one(MIPS_VM *vm, uint32_t instruction);

#endif // VM_H_
