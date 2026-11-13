# MipsSim: MIPS32 Simulator & Assembler

MipsSim is a custom toolchain for the **MIPS32 ISA**, consisting of a **two-pass assembler** and an **emulator** capable of running in web browsers via **WebAssembly (Wasm)**.

Designed with an emphasis on low-level systems engineering, the project implements a hand-written lexer and parser, providing a transparent view of the assembly-to-machine-code pipeline.

> **Status:** work in progress. The integer core of MIPS32 is implemented and covered by tests, but some instructions and behaviors are still missing (see [Limitations](#limitations)).

## Key Features

### Assembler (Two-Pass Architecture)
* **Two-Pass Compilation:**
    * **Pass 1 (Layout):** Calculates memory offsets, resolves symbol labels, and manages sectioning (`.text`, `.data`).
    * **Pass 2 (Encoding):** Handles machine code emission, alignment, and pseudo-instruction expansion.
* **Custom Lexer & Recursive Descent Parser:** Implemented from scratch (without Flex/Bison), with `line:column` error diagnostics.
* **Instruction Formats:** R-type, I-type, J-type and REGIMM-type.

### MIPS32 Emulator
* **Integer ISA:** Arithmetic, logic, shifts, comparisons, HI/LO, branches (including branch-likely), jumps, loads/stores (including unaligned `lwl`/`lwr`/`swl`/`swr`) and traps.
* **Memory:** A single flat 256 KB memory starting at `0x00400000`, holding the code, then the data, with the stack growing down from the top. Word and halfword accesses must be aligned.
* **WebAssembly (Wasm) Runtime:** Compiled with Clang in a **freestanding** configuration (no libc, no Emscripten), using a custom C++ template library.

## Engineering Highlights

* **No STL:** the whole project is built on [`ctl`](ctl/), a custom C++20 library: containers (`Array`, `Slice`), `Maybe`, string building, UTF-8 runes, type traits, and a family of allocators (arena, inline, temporary, scratch, system). It ships its own minimal C++ runtime support (`__cxa_guard_*`, `operator delete`) and is compiled with `-fno-exceptions -fno-rtti -nostdlib++`. Only `printf`-style formatting relies on a vendored library ([stb_sprintf](ctl/src/third_party/stb_sprintf.h)).
* **Freestanding WebAssembly:** the browser build targets `wasm32-unknown-unknown` directly, with no libc and no Emscripten. The JavaScript imports (`console_log`, `exit`, file system stubs) and exports (`run`, `wasm_alloc`, `memory`) are written by hand.
* **stb-style single-header VM:** [`vmips32.h`](vmips32.h) is usable from C or C++ (`#define VMIPS32_IMPLEMENTATION` in one translation unit). The host plugs in through `vmips32_syscall` and `vmips32_trap`, so the same VM runs natively and in the browser.
* **Single source of truth for the ISA:** [`tokens.inl`](tokens.inl) describes every register, instruction (mnemonic, operand format, opcode), pseudo-instruction and directive once. X-macros generate the enums and lookup tables used by the lexer, parser and assembler.
* **Two-pass assembler** with a hand-written lexer and recursive descent parser: errors are accumulated with `line:column` locations instead of stopping at the first one.
* **Regression tests** run with `xmake test`, natively and in GitHub Actions, alongside the automatic deployment of the Wasm build to GitHub Pages.

## Supported Assembly

### Instructions

| Group           | Instructions                                                              |
|-----------------|---------------------------------------------------------------------------|
| Arithmetic      | `add` `addu` `addi` `addiu` `sub` `subu` `mult` `multu` `div` `divu`      |
| Logic           | `and` `or` `xor` `nor` `andi` `ori` `xori` `lui`                          |
| Shifts          | `sll` `srl` `sra` `sllv` `srlv` `srav`                                    |
| Comparisons     | `slt` `sltu` `slti` `sltiu`                                               |
| HI/LO and moves | `mfhi` `mflo` `mthi` `mtlo` `movz` `movn`                                 |
| Branches        | `beq` `bne` `blez` `bgtz` `bltz` `bgez` `bltzal` `bgezal`                 |
| Branch-likely   | `beql` `bnel` `blezl` `bgtzl` `bltzl` `bgezl` `bltzall`                   |
| Jumps           | `j` `jal` `jr` `jalr`                                                     |
| Memory          | `lb` `lbu` `lh` `lhu` `lw` `lwl` `lwr` `sb` `sh` `sw` `swl` `swr`         |
| Traps           | `tge` `tgeu` `tlt` `tltu` `teq` `tne` `tgei` `tlti` `tltiu` `teqi` `tnei` |
| System          | `syscall` `break`                                                         |

### Pseudo-instructions

`nop` `move` `li` `la` `b` `bal` `beqz` `bnez` `neg` `negu` `not` `blt` `ble` `bgt` `bge`

### Directives

`.text` `.data` `.word` `.half` `.byte` `.ascii` `.asciiz` `.space` `.align` `.globl`

### System Calls

| `$v0` | Service         | Arguments                                  |
|-------|-----------------|--------------------------------------------|
| 1     | print integer   | `$a0` = integer                            |
| 4     | print string    | `$a0` = address of a NUL-terminated string |
| 10    | exit            |                                            |
| 11    | print character | `$a0` = character                          |
| 17    | exit with code  | `$a0` = exit code                          |

### Example

```asm
.data
msg: .asciiz "Hello, MIPS!\n"
.text
  li $v0, 4        # print_string
  la $a0, msg
  syscall
  li $v0, 10       # exit
  syscall
```

## Limitations

* **No branch delay slot:** the instruction after a branch or jump is not executed when the branch is taken.
* **Execution always starts at `0x00400000`**, the first instruction of `.text`; the `main` label has no special meaning.
* **`add`, `addi` and `sub` do not trap on overflow**, and **`break` is ignored**.
* **Not implemented:** `mul` and the other SPECIAL2 instructions (`madd`, `msub`, `clo`, `clz`...), `tgeiu`, `bgezall`, `ll`/`sc`, coprocessor 0, the floating-point unit (coprocessor 1), and the MIPS32 Release 2 additions.
* **No input syscalls** (read integer, read string...) and no heap allocation (`sbrk`).

## Tech Stack
* **Language:** C++20 (utilizing a custom template library)
* **Build System:** xmake
* **Targets:** Native & WebAssembly (Wasm). The native build is tested on Linux only; Windows is not supported yet.
* **Toolchain:** GCC or Clang for the native build, Clang/LLVM for Wasm

## Build & Usage

### Native Build
Requires [`xmake`](https://xmake.io) and a C++ compiler (GCC/Clang).

```console
# Clone the repository
git clone https://github.com/Spad0n/mipsim.git
cd mipsim

# Configure and Build
xmake

# Running the simulator
./build/mips <your_program.s>
```

### Tests
Each `tests/<name>.s` with a matching `tests/<name>.expected` is a test: the program is run and its output is compared to the expected file.

```console
xmake test              # run all the tests
xmake test mips/branch  # run a single test
```

`tests/self.s` has no `.expected` file because it must loop forever: run it by hand.

### WebAssembly Build
Requires `clang`, `lld` (`wasm-ld`) and `llvm-ar`.

```console
xmake f -p wasm -a wasm32 --toolchain=wasm32-clang -m release
xmake
# Output: build/mips.wasm, to be served next to index.html
```

### Web Version
The simulator is available to run directly in the browser via Wasm:
https://spad0n.github.io/mipsim/
