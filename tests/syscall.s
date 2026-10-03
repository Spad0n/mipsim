# Intent:   check syscalls 1 (print int), 11 (print char), 4 (print string)
#           and 17 (exit with a code).
# Expected: "-5X", "ok", then the syscall 17 exit message with code 0
.data
msg: .asciiz "ok\n"
.text
  li $a0, -5
  li $v0, 1              # print_int
  syscall
  li $a0, 'X'
  li $v0, 11             # print_char
  syscall
  li $a0, 10             # '\n'
  li $v0, 11
  syscall
  la $a0, msg
  li $v0, 4              # print_string
  syscall
  li $a0, 0              # exit code 0 (a non-zero code fails xmake test)
  li $v0, 17             # exit2
  syscall
