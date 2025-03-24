#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "vm.h"

#define return_defer(x) {result = 1; goto defer;} while(0)

#define ARRAY_LEN(x) (sizeof(x)/sizeof(x[0]))

typedef enum {
    OK_REGISTER,
    OK_NUMBER,
} Oper_Kind;

typedef struct {
    Oper_Kind kind;
    union {
        enum REG_NAME reg;
        int number;
    };
} Operande;

typedef struct {
    enum OP_CODE opcode;
    Operande operande[3];
    size_t operande_count;
} Instruction;

struct {
    const char *name;
    enum REG_NAME reg;
} register_map[] = {
    {"$zero", REG_ZERO},
    {"$at", REG_AT},

    {"$v0", REG_V0},
    {"$v1", REG_V1},

    {"$a0", REG_A0},
    {"$a1", REG_A1},
    {"$a2", REG_A2},
    {"$a3", REG_A3},

    {"$t0", REG_T0},
    {"$t1", REG_T1},
    {"$t2", REG_T2},
    {"$t3", REG_T3},
    {"$t4", REG_T4},
    {"$t5", REG_T5},
    {"$t6", REG_T6},
    {"$t7", REG_T7},

    {"$s0", REG_S0},
    {"$s1", REG_S1},
    {"$s2", REG_S2},
    {"$s3", REG_S3},
    {"$s4", REG_S4},
    {"$s5", REG_S5},
    {"$s6", REG_S6},
    {"$s7", REG_S7},

    {"$t8", REG_T8},
    {"$t9", REG_T9},

    {"$k0", REG_K0},
    {"$k1", REG_K1},

    {"$gp", REG_GP},
    {"$sp", REG_SP},

    {"$s8", REG_S8},
    {"$ra", REG_RA},
};

struct {
    const char *name;
    enum OP_CODE opcode;
} opcode_map[] = {
    {"addi", OP_ADDI},
    {"addiu", OP_ADDIU},

    {"add", OP_ADD},
    {"addu", OP_ADDU},
    {"sub", OP_SUB},
    {"subu", OP_SUBU},

    {"j", OP_JUMP},
    {"jal", OP_JUMP_AND_LINK},

    {"syscall", SYSCALL},
    {"break", BREAK},
};

void trim(char *str) {
    int start = 0;
    while (isspace(str[start])) start++;

    int end = strlen(str) - 1;
    while (end >= 0 && isspace(str[end])) end--;

    int i = 0;
    for (int j = start; j <= end; j++) {
        str[i++] = str[j];
    }
    str[i] = '\0';
}

uint32_t assemble_r_type(enum OP_CODE funct, enum REG_NAME rd, enum REG_NAME rs, enum REG_NAME rt) {
    return (0 << 26) | (rs << 21) | (rt << 16) | (rd << 11) | (0 << 6) | funct;
}

uint32_t assemble_i_type(enum OP_CODE opcode, enum REG_NAME rt, enum REG_NAME rs, uint16_t imm) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | imm;
}

uint32_t assemble_j_type(enum OP_CODE opcode, uint32_t address) {
    return (opcode << 26) | (address & 0x3FFFFFF);
}

enum REG_NAME parse_register(const char *token) {
    for (size_t i = 0; i < ARRAY_LEN(register_map); i++) {
        if (strcmp(token, register_map[i].name) == 0) {
            return register_map[i].reg;
        }
    }
    fprintf(stderr, "Unknown register: %s\n", token);
    abort();
}

enum OP_CODE parse_opcode(const char *token) {
    for (size_t i = 0; i < ARRAY_LEN(opcode_map); i++) {
        if (strcmp(token, opcode_map[i].name) == 0) {
            return opcode_map[i].opcode;
        }
    }
    fprintf(stderr, "Unknown register: %s\n", token);
    abort();
}

int parse_number(const char *token) {
    size_t length = strlen(token);
    for (size_t i = 0; i < length; i++) {
        if (!isdigit(token[i])) {
            fprintf(stderr, "could not parse number: %s\n", token);
            abort();
        }
    }
    return atoi(token);
}

Instruction parse_line(char *line) {
    Instruction instr = {0};
    char *token = strtok(line, " ");
    if (token != NULL) {
        instr.opcode = parse_opcode(token);
        size_t i = 0;
        switch (instr.opcode) {
        case OP_ADD:
        case OP_SUB:
            while ((token = strtok(NULL, ",")) != NULL && i < 3) {
                trim(token);
                instr.operande[i].kind = OK_REGISTER;
                instr.operande[i].reg = parse_register(token);
                i += 1;
            } break;
        case OP_ADDI:
        case OP_ADDIU:
            while ((token = strtok(NULL, ",")) != NULL && i < 3) {
                trim(token);
                if (i < 2) {
                    instr.operande[i].kind = OK_REGISTER;
                    instr.operande[i].reg = parse_register(token);
                } else {
                    instr.operande[i].kind = OK_NUMBER;
                    instr.operande[i].number = parse_number(token);
                }
                i += 1;
            }
        case SYSCALL: break;
        default:
            fprintf(stderr, "Unknown operande: %s\n", token);
            abort();
        }
        instr.operande_count = i;
    }
    return instr;
}

int main(int argc, char **argv) {
    int result = 0;
    FILE *f = NULL;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.s>\n", argv[0]);
        return_defer(1);
    }
    const char *file_path = argv[1];
    MIPS_VM vm = {0};

    f = fopen(file_path, "r");
    if (!f) {
        fprintf(stderr, "Could not open the file: %s\n", file_path);
        return_defer(1);
    }
    char line[2048];
    #if 0
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        Instruction instr = parse_line(line);
        printf("%s ", vm_get_opcode_name(instr.opcode));
        for (size_t j = 0; j < instr.operande_count; j++) {
            if (instr.operande[j].kind == OK_REGISTER) {
                printf("%s ", vm_get_reg_name(instr.operande[j].reg));
            } else if (instr.operande[j].kind == OK_NUMBER) {
                printf("%d ", instr.operande[j].number);
            }
        }
        printf("\n");
    }
    #else
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        Instruction instr = parse_line(line);
        switch (instr.opcode) {
        case OP_ADDI:
            vm_execute_one(&vm, assemble_i_type(OP_ADDI, instr.operande[0].reg, instr.operande[1].reg, instr.operande[2].number));
            break;
        case OP_ADD:
            vm_execute_one(&vm, assemble_r_type(OP_ADD, instr.operande[0].reg, instr.operande[1].reg, instr.operande[2].reg));
            break;
        case OP_SUB:
            vm_execute_one(&vm, assemble_r_type(OP_SUB, instr.operande[0].reg, instr.operande[1].reg, instr.operande[2].reg));
            break;
        case SYSCALL:
            vm_execute_one(&vm, SYSCALL);
            break;
        default:
            fprintf(stderr, "instruction not implemented yet: %s\n", vm_get_opcode_name(instr.opcode));
            return_defer(1);
        }
    }
    #endif

 defer:
    if (f) fclose(f);
    return result;
}
