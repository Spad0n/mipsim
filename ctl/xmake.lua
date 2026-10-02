-- ctl: lightweight C++ library without the standard library

-- in case there is some bugs
option("ctl_no_stdlibcxx")
    set_default(true)
    set_showmenu(true)
    set_description("Disable C++ standard library")
option_end()

target("ctl")
    set_kind("static")
    set_languages("c++20")

    -- system_posix.cpp, system_wasm.cpp and system_windows.cpp are
    -- #included by system.cpp, they must not be compiled on their own
    add_files(
        "src/allocator.cpp",
        "src/cpprt.cpp",
        "src/file.cpp",
        "src/stream.cpp",
        "src/string.cpp",
        "src/system.cpp",
        "src/unicode.cpp"
    )
    add_includedirs("src", {public = true})

    if is_plat("windows") then
        add_cxxflags(
            "/EHs-",   -- disable exceptions
            "/GR-",    -- disable RTTI
            "/wd4530", -- disable warning "C++ exception handler used..." if necessary
            {public = true, tools = {"cl", "clang_cl"}}
        )
    end
    add_cxxflags("-fno-exceptions", "-fno-rtti", {public = true, tools = {"gcc", "gxx", "clang", "clangxx"}})
    if has_config("ctl_no_stdlibcxx") then
        add_ldflags("-nostdlib++", {public = true, force = true, tools = {"gcc", "gxx", "clang", "clangxx"}})
    end

    if is_plat("wasm") then
        add_cxxflags(
            "-nostdlibinc",  -- no system headers, but only use builtin clang
            "-msimd128",     -- active <wasm_simd128.h> for allocator.cpp
            "-mbulk-memory", -- active memory.fill / memory.copy
            {public = true}
        )
    end
target_end()
