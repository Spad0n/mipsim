cmake_minimum_required(VERSION 3.25)

project(ctl-playground
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CTL_NO_STDLIBCXX ON)

#option(USE_ASAN "Enable Address Sanitizer" OFF)
#
#if(USE_ASAN)
#  add_library(asan_config INTERFACE)
#  if (MSVC)
#    target_compile_options(asan_config INTERFACE /fsanitize=address)
#    target_link_options(asan_config INTERFACE /fsanitize=address)
#  else()
#    target_compile_options(asan_config INTERFACE
#      -fsanitize=address
#      -fno-omit-frame-pointer
#    )
#    target_link_options(asan_config INTERFACE -fsanitize=address)
#  endif()
#  message(STATUS "ASan configuration created")
#endif()

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
