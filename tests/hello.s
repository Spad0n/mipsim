# Intent:   syscall 4 prints a NUL-terminated string from the .data segment.
# Expected: "Hello" followed by a newline
.data
msg: .asciiz "Hello\n"
.text
  li $v0, 4              # print_string
  la $a0, msg            # address of the string
  syscall
  li $v0, 10             # exit
  syscall
