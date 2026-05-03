#include "ctl/system.hpp"
#include "ctl/allocator.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include <stdio.h>
#include <stdlib.h>

#define  VMIPS32_IMPLEMENTATION
#include "vmips32.h"

extern "C" {
    
    void vmips32_syscall(uint32_t v0, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) {
        switch (v0) {
        case 1: // Print Integer
            printf("%d", (int32_t)a0);
            break;

        case 4: { // Print String
            uint32_t addr = a0;
            while (1) {
                uint32_t* cell = get_memory_ptr(addr);
                if (!cell) break;

                uint8_t* bytes = (uint8_t*)cell;
                
                // It compensate the inversion Little endian of the processor host
                char c = (char)bytes[3 - addr % 4];

                if (c == '\0') break;
                putchar(c);
                addr += 1;
            }
            break;
        }

        case 10: // Exit standard
            printf("\n--- Program exited (syscall 10) ---\n");
            exit(0);
            break;

        case 17: // Exit with value
            printf("\n--- Program exited with code %d (syscall 17) ---\n", a0);
            exit(a0);
            break;

        case 11: // Print Character
            putchar((char)(a0 & 0xFF));
            break;

        default:
            // Debug : check which syscall is called
            printf("\n[Syscall] Unknown syscall code: %u (a0: %u)\n", v0, a0);
            break;
        }
    }

    void vmips32_trap(const char *message) {
        fprintf(stderr, "%s", message);
        exit(1);
    }
}

using namespace ctl;

Sint32 main() {
    SystemAllocator sys;
    TemporaryAllocator alloc{sys};

    StringView src = 
        ".data\n"
        "    msg_start:  .asciiz \"Calcul de 5! : \"\n"
        "    msg_bit:    .asciiz \"Test Bitwise: \"\n"
        "    msg_res:    .asciiz \"\\nResultat final: \"\n"
        "    storage:    .word 0\n"
        "\n"
        ".text\n"
        ".globl main\n"
        "\n"
        "main:\n"
        "    li $sp, 0x00401000\n"
        "\n"
        "    la $a0, msg_start\n"
        "    li $v0, 4\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "    li $t0, 0xAA\n"
        "    li $t1, 0x55\n"
        "    or $t2, $t0, $t1\n"
        "    and $t3, $t2, $t0\n"
        "    xor $t4, $t3, $t0\n"
        "\n"
        "    li $s0, 0\n"
        "    bnez $t4, fail_bit\n"
        "    nop\n"
        "    li $s0, 1\n"
        "\n"
        "fail_bit:\n"
        "    la $a0, msg_bit\n"
        "    li $v0, 4\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "    move $a0, $s0\n"
        "    li $v0, 1\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "    li $a0, 5\n"
        "    jal factorial\n"
        "    nop\n"
        "\n"
        "    la $t8, storage\n"
        "    sw $v0, 0($t8)\n"
        "\n"
        "    la $a0, msg_res\n"
        "    li $v0, 4\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "    lw $a0, 0($t8)\n"
        "    li $v0, 1\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "    li $v0, 17\n"
        "    li $a0, 69\n"
        "    syscall\n"
        "    nop\n"
        "\n"
        "factorial:\n"
        "    addiu $sp, $sp, -8\n"
        "    sw $ra, 4($sp)\n"
        "    sw $a0, 0($sp)\n"
        "\n"
        "    li $t0, 1\n"
        "    ble $a0, $t0, fact_base\n"
        "    nop\n"
        "\n"
        "    addiu $a0, $a0, -1\n"
        "    jal factorial\n"
        "    nop\n"
        "\n"
        "    lw $a0, 0($sp)\n"
        "    lw $ra, 4($sp)\n"
        "    addiu $sp, $sp, 8\n"
        "    mult $v0, $a0\n"
        "    mflo $v0\n"
        "    jr $ra\n"
        "    nop\n"
        "\n"
        "fact_base:\n"
        "    li $v0, 1\n"
        "    addiu $sp, $sp, 8\n"
        "    jr $ra\n"
        "    nop\n";

    Lexer lex{src, alloc};
    auto tokens = lex.tokenize();

    if (!tokens || lex.has_error()) {
        StringView msg = lex.error_message();
        fprintf(stderr, "ERROR: %.*s\n", msg.length(), msg.data());
        return 1;
    }

    Parser parser{tokens->slice(), alloc};
    parser.parse();
    if (parser.has_errors()) {
        for (auto& e : parser.errors()) {
            Location loc = e.loc;
            StringView msg = e.message;
            fprintf(stderr, "error:%d:%d: %.*s\n", loc.line, loc.column, msg.length(), msg.data());
        }
        return 1;
    }

    Assembler asm_{alloc};
    asm_.assemble(parser.stmts(), parser.operands());
    if (asm_.has_errors()) {
        for (auto& e : asm_.errors()) {
            Location loc = e.loc;
            StringView msg = e.message;
            fprintf(stderr, "error:%d:%d: %.*s\n", loc.line, loc.column, msg.length(), msg.data());
        }
        return 1;
    }

    auto bc = asm_.bytecode();

    vmips32_load_program(bc.data(), bc.length() * 4);

    for (;;) {
        vmips32_step();
    }
}
