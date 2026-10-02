set_project("mipsim")
set_xmakever("2.8.0")

add_rules("mode.debug", "mode.release")
set_languages("c++20")

-- clang targeting wasm32 without libc (no emscripten, no wasi-sdk)
toolchain("wasm32-clang")
    set_kind("standalone")
    set_toolset("cc", "clang")
    set_toolset("cxx", "clang++", "clang")
    set_toolset("ld", "clang++", "clang")
    set_toolset("ar", "llvm-ar", "ar")
    on_check(function (toolchain)
        import("lib.detect.find_tool")
        return find_tool("clang") and find_tool("clang++")
    end)
    on_load(function (toolchain)
        toolchain:add("cxflags", "--target=wasm32-unknown-unknown")
        toolchain:add("ldflags", "--target=wasm32-unknown-unknown")
    end)
toolchain_end()

if is_plat("wasm") then
    set_toolchains("wasm32-clang")
end

includes("ctl")

target("mips")
    set_kind("binary")
    set_targetdir("build")
    add_files(
        "main.cpp",
        "lexer.cpp",
        "token.cpp",
        "parser.cpp",
        "assembler.cpp"
    )
    add_deps("ctl")

    if is_plat("wasm") then
        set_extension(".wasm")
        add_ldflags(
            "-nostdlib",
            "-Wl,--no-entry",
            "-Wl,--allow-undefined",
            "-Wl,--export=memory",
            "-Wl,--export=__heap_base",
            "-Wl,--export=__data_end",
            "-Wl,-z,stack-size=8388608",
            "-Wl,--export=run",
            "-Wl,--export=wasm_alloc",
            {force = true}
        )
    end
target_end()
