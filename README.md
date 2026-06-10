# MipsSim: MIPS32 Simulator & Assembler

MipsSim is a custom toolchain for the **MIPS32 ISA**, consisting of a **two-pass assembler** and an **emulator** capable of running in web browsers via **WebAssembly (Wasm)**.

Designed with an emphasis on low-level systems engineering, the project implements a hand-written lexer and parser, providing a transparent view of the assembly-to-machine-code pipeline.

## Key Features

### Assembler (Two-Pass Architecture)
* **Two-Pass Compilation:**
    * **Pass 1 (Layout):** Calculates memory offsets, resolves symbol labels, and manages sectioning (`.text`, `.data`).
    * **Pass 2 (Encoding):** Handles machine code emission, instruction alignment, and pseudo-instruction expansion (e.g., `li`, `la`, `move`, `blt`).
* **Custom Lexer & Recursive Descent Parser:** Implemented from scratch (without Flex/Bison) to provide precise error diagnostics and full control over instruction decoding.
* **ISA Coverage:** Full support for R-type, I-type, J-type, and REGIMM-type instruction formats.

### MIPS32 Emulator
* **ISA Implementation:** Precise emulation of the MIPS32 instruction set, including branching logic, arithmetic operations, and system calls.
* **Memory Management:** Implements a segmented memory model with strict alignment checks and support for unaligned access handling.
* **WebAssembly (Wasm) Runtime:** Compiled using a **freestanding** configuration with Clang, allowing direct interop between the MIPS bytecode and the browser runtime.

## Tech Stack
* **Language:** C++20 (utilizing a custom template library)
* **Build System:** CMake
* **Targets:** Native (Linux/macOS/Windows) & WebAssembly (Wasm)
* **Toolchain:** Clang/LLVM

## Build & Usage

### Native Build
Requires `cmake` and a C++ compiler (GCC/Clang).

```console
# Clone the repository
git clone https://github.com/Spad0n/mipsim.git
cd mipsim

# Configure and Build
cmake -B build
cmake --build build

# Running the simulator
./mips <your_program.mips>
```

### Web Version
The simulator is available to run directly in the browser via Wasm:
https://spad0n.github.io/mipsim/
