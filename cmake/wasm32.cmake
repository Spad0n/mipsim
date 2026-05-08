set(CMAKE_SYSTEM_NAME       WASI)
set(CMAKE_SYSTEM_PROCESSOR  wasm32)

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
#set(CMAKE_AR           llvm-ar)
#set(CMAKE_RANLIB       llvm-ranlib)

set(CMAKE_C_COMPILER_TARGET   wasm32-unknown-unknown)
set(CMAKE_CXX_COMPILER_TARGET wasm32-unknown-unknown)

# Les try_compile internes de CMake ne peuvent pas linker un .wasm
# sans nos flags. On bascule en mode static lib pour ces tests.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
