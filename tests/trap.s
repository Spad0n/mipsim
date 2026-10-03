# Intent:   check tge, tgeu, tlt, tltu, teq, tne, tgei, tlti, tltiu, teqi and
#           tnei when the condition is false: no trap, execution continues.
#           A triggered trap stops the program with exit code 1, which
#           xmake test always counts as a failure, so it is not tested here.
# Expected: 13 (number of trap instructions passed)
.text
  li $t0, 1
  li $t1, 2
  li $t2, -1             # 0xFFFFFFFF
  li $s0, 0              # counter

  tge $t0, $t1           # 1 >= 2: false
  addiu $s0, $s0, 1
  tgeu $t0, $t1          # 1 >= 2: false
  addiu $s0, $s0, 1
  tgeu $t1, $t2          # 2 >= 0xFFFFFFFF (unsigned): false
  addiu $s0, $s0, 1
  tlt $t1, $t0           # 2 < 1: false
  addiu $s0, $s0, 1
  tlt $t1, $t2           # 2 < -1 (signed): false
  addiu $s0, $s0, 1
  tltu $t1, $t0          # 2 < 1: false
  addiu $s0, $s0, 1
  teq $t0, $t1           # 1 == 2: false
  addiu $s0, $s0, 1
  tne $t0, $t0           # 1 != 1: false
  addiu $s0, $s0, 1

  tgei $t0, 2            # 1 >= 2: false
  addiu $s0, $s0, 1
  tlti $t0, 0            # 1 < 0: false
  addiu $s0, $s0, 1
  tltiu $t0, 1           # 1 < 1: false
  addiu $s0, $s0, 1
  teqi $t0, 5            # 1 == 5: false
  addiu $s0, $s0, 1
  tnei $t0, 1            # 1 != 1: false
  addiu $s0, $s0, 1

  move $a0, $s0
  jal print_int          # 13
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
