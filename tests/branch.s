# Intent:   check beq, bne, blez, bgtz, bltz and bgez, taken and not taken,
#           and backward branches (negative offset) in loops.
#           Each check prints 1 if the branch is taken, 0 otherwise:
#             li $a0, 1 / branch / li $a0, 0 / label: print
# Expected: 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 15, 3
.text
  li $t0, 0
  li $t1, 1
  li $t2, -1

  li $a0, 1
  beq $t1, $t1, beq_t
  li $a0, 0              # executed only if not taken
beq_t:
  jal print_int
  li $a0, 1
  beq $t0, $t1, beq_n
  li $a0, 0              # executed only if not taken
beq_n:
  jal print_int
  li $a0, 1
  bne $t0, $t1, bne_t
  li $a0, 0              # executed only if not taken
bne_t:
  jal print_int
  li $a0, 1
  bne $t1, $t1, bne_n
  li $a0, 0              # executed only if not taken
bne_n:
  jal print_int
  li $a0, 1
  blez $t0, blez_z
  li $a0, 0              # executed only if not taken
blez_z:
  jal print_int
  li $a0, 1
  blez $t2, blez_neg
  li $a0, 0              # executed only if not taken
blez_neg:
  jal print_int
  li $a0, 1
  blez $t1, blez_n
  li $a0, 0              # executed only if not taken
blez_n:
  jal print_int
  li $a0, 1
  bgtz $t1, bgtz_t
  li $a0, 0              # executed only if not taken
bgtz_t:
  jal print_int
  li $a0, 1
  bgtz $t0, bgtz_n
  li $a0, 0              # executed only if not taken
bgtz_n:
  jal print_int
  li $a0, 1
  bltz $t2, bltz_t
  li $a0, 0              # executed only if not taken
bltz_t:
  jal print_int
  li $a0, 1
  bltz $t0, bltz_n
  li $a0, 0              # executed only if not taken
bltz_n:
  jal print_int
  li $a0, 1
  bgez $t0, bgez_t
  li $a0, 0              # executed only if not taken
bgez_t:
  jal print_int
  li $a0, 1
  bgez $t2, bgez_n
  li $a0, 0              # executed only if not taken
bgez_n:
  jal print_int

  # backward bne: 5 + 4 + 3 + 2 + 1
  li $t0, 5
  li $a0, 0
sum_loop:
  addu $a0, $a0, $t0
  addiu $t0, $t0, -1
  bne $t0, $zero, sum_loop
  jal print_int          # 15

  # backward bgtz: counts 3 iterations
  li $t0, 3
  li $a0, 0
count_loop:
  addiu $a0, $a0, 1
  addiu $t0, $t0, -1
  bgtz $t0, count_loop
  jal print_int          # 3

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
