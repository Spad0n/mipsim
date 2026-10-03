# Intent:   check the branch pseudo-instructions b, beqz, bnez, blt, ble, bgt
#           and bge (expanded with slt + beq/bne through $at), taken and not
#           taken. blt is also checked with a negative (signed) value.
#           Each check prints 1 if the branch is taken, 0 otherwise:
#             li $a0, 1 / branch / li $a0, 0 / label: print
# Expected: 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0
.text
  li $t0, 1
  li $t1, 2
  li $t2, -1

  li $a0, 1
  b b_t
  li $a0, 0              # executed only if not taken
b_t:
  jal print_int
  li $a0, 1
  beqz $zero, beqz_t
  li $a0, 0              # executed only if not taken
beqz_t:
  jal print_int
  li $a0, 1
  beqz $t0, beqz_n
  li $a0, 0              # executed only if not taken
beqz_n:
  jal print_int
  li $a0, 1
  bnez $t0, bnez_t
  li $a0, 0              # executed only if not taken
bnez_t:
  jal print_int
  li $a0, 1
  bnez $zero, bnez_n
  li $a0, 0              # executed only if not taken
bnez_n:
  jal print_int
  li $a0, 1
  blt $t0, $t1, blt_t
  li $a0, 0              # executed only if not taken
blt_t:
  jal print_int
  li $a0, 1
  blt $t1, $t0, blt_n
  li $a0, 0              # executed only if not taken
blt_n:
  jal print_int
  li $a0, 1
  blt $t2, $t0, blt_neg
  li $a0, 0              # executed only if not taken
blt_neg:
  jal print_int
  li $a0, 1
  ble $t0, $t0, ble_t
  li $a0, 0              # executed only if not taken
ble_t:
  jal print_int
  li $a0, 1
  ble $t1, $t0, ble_n
  li $a0, 0              # executed only if not taken
ble_n:
  jal print_int
  li $a0, 1
  bgt $t1, $t0, bgt_t
  li $a0, 0              # executed only if not taken
bgt_t:
  jal print_int
  li $a0, 1
  bgt $t0, $t0, bgt_n
  li $a0, 0              # executed only if not taken
bgt_n:
  jal print_int
  li $a0, 1
  bge $t0, $t0, bge_t
  li $a0, 0              # executed only if not taken
bge_t:
  jal print_int
  li $a0, 1
  bge $t0, $t1, bge_n
  li $a0, 0              # executed only if not taken
bge_n:
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
