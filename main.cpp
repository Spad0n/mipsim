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

        case 4: { // Print String (Version sécurisée et neutre en Endianness)
            uint32_t addr = a0;
            while (1) {
                uint32_t* cell = get_memory_ptr(addr);
                if (!cell) break;

                // On accède à la cellule comme un tableau de 4 octets
                // Cela évite de se soucier de l'endianness du processeur hôte
                uint8_t* bytes = (uint8_t*)cell;
                
                // MIPS est traditionnellement Big Endian, donc l'octet 0 est à l'adresse haute
                // Mais cela dépend de la façon dont votre 'load_program' remplit le tableau.
                // Si vous avez copié des octets brut, addr % 4 vous donne l'index direct :
                char c = (char)bytes[addr % 4];

                if (c == '\0') break;
                putchar(c);
                addr++;
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
            // Debug : savoir quel syscall inconnu est appelé
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
        "lui $sp, 0x0040\n"
        "ori $sp, $sp, 0x1000\n"
        "addiu $v0, $zero, 4\n"
        "lui $a0, 0x0040\n"
        "ori $a0, $a0, 0x0200\n"
        "syscall\n"
        "ori $t0, $zero, 0xAA\n"
        "ori $t1, $zero, 0x55\n"
        "or $t2, $t0, $t1\n"
        "and $t3, $t2, $t0\n"
        "xor $t4, $t3, $t0\n"
        "sltiu $a0, $t4, 1\n"
        "addu $s0, $a0, $zero\n"
        "addiu $v0, $zero, 4\n"
        "lui $a0, 0x0040\n"
        "ori $a0, $a0, 0x0240\n"
        "syscall\n"
        "addiu $v0, $zero, 1\n"
        "addu $a0, $s0, $zero\n"
        "syscall\n"
        "addiu $a0, $zero, 5\n"
        "jal factorial\n"
        "lui $t8, 0x0040\n"
        "sw $v0, 0x0280($t8)\n"
        "addiu $v0, $zero, 4\n"
        "lui $a0, 0x0040\n"
        "ori $a0, $a0, 0x0220\n"
        "syscall\n"
        "lui $t8, 0x0040\n"
        "lw $a0, 0x0280($t8)\n"
        "addiu $v0, $zero, 1\n"
        "syscall\n"
        "addiu $v0, $zero, 17\n"
        "addiu $a0, $zero, 69\n"
        "syscall\n"
        "factorial:\n"
        "addiu $sp, $sp, -8\n"
        "sw $ra, 4($sp)\n"
        "sw $a0, 0($sp)\n"
        "slti $t0, $a0, 2\n"
        "beq $t0, $zero, recurse\n"
        "addiu $v0, $zero, 1\n"
        "addiu $sp, $sp, 8\n"
        "jr $ra\n"
        "recurse:\n"
        "addiu $a0, $a0, -1\n"
        "jal factorial\n"
        "lw $a0, 0($sp)\n"
        "lw $ra, 4($sp)\n"
        "addiu $sp, $sp, 8\n"
        "mult $v0, $a0\n"
        "mflo $v0\n"
        "jr $ra";

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
