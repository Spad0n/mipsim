# Intent:   check add, addu, sub, subu, addi and addiu, including the
#           32-bit wrap-around of the unsigned variants (no trap).
# Expected: 4, 4, 10, -10, -3, 32774, -2147483648, 2147483647, 2147483647
.text
  li $t0, 7
  li $t1, -3

  add $a0, $t0, $t1      # 7 + -3
  jal print_int
  addu $a0, $t0, $t1     # 7 + -3
  jal print_int
  sub $a0, $t0, $t1      # 7 - -3
  jal print_int
  subu $a0, $t1, $t0     # -3 - 7
  jal print_int
  addi $a0, $t0, -10     # negative immediate is sign-extended
  jal print_int
  addiu $a0, $t0, 32767  # largest signed 16-bit immediate
  jal print_int

  li $t2, 0x7FFFFFFF     # INT_MAX
  li $t3, 1
  addu $a0, $t2, $t3     # wraps to INT_MIN
  jal print_int
  li $t2, 0x80000000     # INT_MIN
  subu $a0, $t2, $t3     # wraps to INT_MAX
  jal print_int
  addiu $a0, $t2, -1     # wraps to INT_MAX
  jal print_int

  li $v0, 10             # exit
  syscall

# print_int: prints $a0 as a signed integer, then a newline
print_int:
  li $v0, 1
  syscall
  li $a0, 10             # '\n'
  li $v0, 11
  syscall
  jr $ra
