#include "ctl/system.hpp"
#include "ctl/allocator.hpp"
#include "ctl/file.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include <unistd.h>

#define  VMIPS32_IMPLEMENTATION
#include "vmips32.h"

extern "C" {
    
    void vmips32_syscall(uint32_t v0, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) {
        InlineAllocator<2048> alloc;
        StringBuilder sb{alloc};
        switch (v0) {
        case 1: // Print Integer
            sb.put((int32_t)a0);
            Console::print(*sb.result());
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
                Console::print(StringView(&c));
                addr += 1;
            }
            break;
        }

        case 10: // Exit standard
            Console::print("\n--- Program exited (syscall 10) ---\n");
            _exit(0);
            break;

        case 17: // Exit with value
            sb.format("\n--- Program exited with code %d (syscall 17) ---\n", a0);
            Console::print(*sb.result());
            _exit(a0);
            break;

        case 11: // Print Character
            sb.put((char)(a0 & 0xFF));
            Console::print(*sb.result());
            break;

        default:
            // Debug : check which syscall is called
            sb.format("\n[Syscall] Unknown syscall code: %u (a0: %u)\n", v0, a0);
            Console::print(*sb.result());
            break;
        }
    }

    void vmips32_trap(const char *message) {
        Console::print(StringView(message));
        _exit(1);
    }
}

using namespace ctl;

Sint32 main(Sint32 argc, char **argv) {
    SystemAllocator sys;
    TemporaryAllocator alloc{sys};

    if (argc < 2) {
        Console::print("Usage: ./mips <file.mips>\n");
        return 1;
    }

    auto file = File::open(argv[1], File::Access::RD);
    if (!file) {
        Console::print("ERROR: could not open the file\n");
        return 1;
    }
    Array<Uint8> content = file->map(alloc);
    StringView src = content.slice().cast<const char>();

    Lexer lex{src, alloc};
    auto tokens = lex.tokenize();

    StringBuilder sb{alloc};

    if (!tokens || lex.has_error()) {
        StringView msg = lex.error_message();
        Location loc = lex.error_location();
        sb.format("%s:%d:%d: %.*s\n", argv[1], loc.line, loc.column, msg.length(), msg.data());
        Console::print(*sb.result());
        return 1;
    }

    Parser parser{tokens->slice(), alloc};
    parser.parse();
    if (parser.has_errors()) {
        for (auto& e : parser.errors()) {
            sb.reset();
            Location loc = e.loc;
            StringView msg = e.message;
            sb.format("%s:%d:%d: %.*s\n", argv[1], loc.line, loc.column, msg.length(), msg.data());
            Console::print(*sb.result());
        }
        return 1;
    }

    Assembler asm_{alloc};
    asm_.assemble(parser.stmts(), parser.operands());
    if (asm_.has_errors()) {
        for (auto& e : asm_.errors()) {
            sb.reset();
            Location loc = e.loc;
            StringView msg = e.message;
            sb.format("%s:%d:%d: %.*s\n", argv[1], loc.line, loc.column, msg.length(), msg.data());
            Console::print(*sb.result());
        }
        return 1;
    }

    auto bc = asm_.bytecode();

    vmips32_load_program(bc.data(), bc.length() * 4);

    for (;;) {
        vmips32_step();
    }
}
