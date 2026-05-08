# cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/wasm32.cmake
cmake_minimum_required(VERSION 3.25)

project(ctl-playground
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CTL_NO_STDLIBCXX ON)

add_executable(mips
  main.cpp
  lexer.cpp
  token.cpp
  parser.cpp
  assembler.cpp
)

target_link_libraries(mips PRIVATE
  ctl
)

add_subdirectory(ctl)

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set_target_properties(mips PROPERTIES SUFFIX ".wasm")

  target_link_options(mips PRIVATE
    -nostdlib
    -Wl,--no-entry
    -Wl,--allow-undefined
    -Wl,--export=memory
    -Wl,--export=__heap_base
    -Wl,--export=__data_end
    -Wl,-z,stack-size=8388608
    -Wl,--export=run
    -Wl,--export=wasm_alloc
  )
endif()
