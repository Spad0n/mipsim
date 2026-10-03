# Intent:   check slt, sltu, slti and sltiu: signed and unsigned comparisons
#           give opposite results for -1 (0xFFFFFFFF) and 1.
# Expected: 1, 0, 0, 1, 1, 0, 1, 0
.text
  li $t0, -1             # 0xFFFFFFFF
  li $t1, 1

  slt $a0, $t0, $t1      # -1 < 1 (signed)
  jal print_int
  slt $a0, $t1, $t0      # 1 < -1
  jal print_int
  sltu $a0, $t0, $t1     # 0xFFFFFFFF < 1 (unsigned)
  jal print_int
  sltu $a0, $t1, $t0     # 1 < 0xFFFFFFFF
  jal print_int

  slti $a0, $t0, 0       # -1 < 0
  jal print_int
  slti $a0, $t1, 1       # 1 < 1
  jal print_int
  sltiu $a0, $t1, -1     # 1 < 0xFFFFFFFF: the immediate is sign-extended
  jal print_int
  sltiu $a0, $t0, 5      # 0xFFFFFFFF < 5
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
