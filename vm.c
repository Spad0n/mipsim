//#define NOB_STRIP_PREFIX
//#include "nob.h"
#include <stdio.h>
#include <assert.h>
#include "vm.h"

const char* vm_get_opcode_name(enum OP_CODE opcode) {
    switch (opcode) {
    case OP_ADDI:          return "OP_ADDI";
    case OP_ADDIU:         return "OP_ADDIU";

    case OP_ADD:           return "OP_ADD";
    case OP_ADDU:          return "OP_ADDU";
    case OP_SUB:           return "OP_SUB";
    case OP_SUBU:          return "OP_SUBU";

    case OP_JUMP:          return "OP_JUMP";
    case OP_JUMP_AND_LINK: return "OP_JUMP_AND_LINK";
    case SYSCALL:          return "SYSCALL";
    case BREAK:            return "BREAK";
    default:
        assert(0 && "should not crash");
    }
}

const char* vm_get_reg_name(enum REG_NAME reg) {
    switch (reg) {
    case REG_ZERO: return "REG_ZERO";
    case REG_AT: return "REG_AT";

    case REG_V0: return "REG_V0";
    case REG_V1: return "REG_V1";

    case REG_A0: return "REG_A0";
    case REG_A1: return "REG_A1";
    case REG_A2: return "REG_A2";
    case REG_A3: return "REG_A3";

    case REG_T0: return "REG_T0";
    case REG_T1: return "REG_T1";
    case REG_T2: return "REG_T2";
    case REG_T3: return "REG_T3";
    case REG_T4: return "REG_T4";
    case REG_T5: return "REG_T5";
    case REG_T6: return "REG_T6";
    case REG_T7: return "REG_T7";

    case REG_S0: return "REG_S0";
    case REG_S1: return "REG_S1";
    case REG_S2: return "REG_S2";
    case REG_S3: return "REG_S3";
    case REG_S4: return "REG_S4";
    case REG_S5: return "REG_S5";
    case REG_S6: return "REG_S6";
    case REG_S7: return "REG_S7";

    case REG_T8: return "REG_T8";
    case REG_T9: return "REG_T9";

    case REG_K0: return "REG_K0";
    case REG_K1: return "REG_K1";

    case REG_GP: return "REG_GP";
    case REG_SP: return "REG_SP";

    case REG_S8: return "REG_S8";
    case REG_RA: return "REG_RA";
    default:
        assert(0 && "Unknown register");
    }
}

static void vm_syscall(MIPS_VM *vm) {
    switch(vm->regs[REG_V0]) {
    case 0:
        printf("%d\n", vm->regs[REG_A0]); return;
    default:
        
    }
}

static void vm_execute_rtype(MIPS_VM *vm, uint32_t instruction) {
    enum OP_CODE funct = instruction & 0x3F;
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int rd = (instruction >> 11) & 0x1F;

    switch (funct) {
    case OP_ADD:
    case OP_ADDU:
        vm->regs[rd] = vm->regs[rs] + vm->regs[rt]; return;
    case OP_SUB:
    case OP_SUBU:
        vm->regs[rd] = vm->regs[rs] - vm->regs[rt]; return;
    case SYSCALL: // Special instruction
        vm_syscall(vm); return;
    default:
        assert(0 && "Unknown instruction for rtype");
    }
}

static void vm_execute_itype(MIPS_VM *vm, uint32_t instruction) {
    enum OP_CODE opcode = instruction >> 26 & 0x3F;
    int rs = (instruction >> 21) & 0x1F;
    int rt = (instruction >> 16) & 0x1F;
    int imm = instruction & 0xFFFF;

    switch (opcode) {
    case OP_ADDI:
    case OP_ADDIU:
        vm->regs[rt] = vm->regs[rs] + imm; return;
    default:
        assert(0 && "Unknown instruction for itype");
    }
}

static void vm_execute_jtype(MIPS_VM *vm, uint32_t instruction) {
    (void)vm;
    (void)instruction;
    assert(0 && "implement jtypes");
}

void vm_execute_many(MIPS_VM *vm, uint32_t *instructions, size_t num_instructions) {
    while (vm->PC < num_instructions) {
        uint32_t instruction = instructions[vm->PC];
        int opcode = (instruction >> 26) & 0x3F;

        switch (opcode) {
        case 0b000000:
            vm_execute_rtype(vm, instruction); break;
        case OP_ADDI:
        case OP_ADDIU:
            vm_execute_itype(vm, instruction); break;
        case OP_JUMP:
        case OP_JUMP_AND_LINK:
            vm_execute_jtype(vm, instruction); break;
        default:
            assert(0 && "Unknown instruction");
        }

        vm->PC += 1;
    }
}

void vm_execute_one(MIPS_VM *vm, uint32_t instruction) {
    int opcode = (instruction >> 26) & 0x3F;

    switch (opcode) {
    case 0b000000:
        vm_execute_rtype(vm, instruction); return;
    case OP_ADDI:
    case OP_ADDIU:
        vm_execute_itype(vm, instruction); return;
    case OP_JUMP:
    case OP_JUMP_AND_LINK:
        vm_execute_jtype(vm, instruction); return;
    default:
        assert(0 && "Unknown instruction");
    }
    vm->PC += 1;
}
