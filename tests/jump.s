# Intent:   check j, jr, jalr and the "and link" branches bgezal, bltzal and
#           bltzall. The functions are placed before main, so every call is
#           a backward jump (negative branch offset).
# Expected: 1, 1, 15, 8, 30, 40, 60, 70, 90, 100
.text
  j main

# add10: $a0 += 10, returns through $ra
add10:
  addiu $a0, $a0, 10
  jr $ra

# add1_s0: $a0 += 1, returns through $s0 (used with jalr $s0, ...)
add1_s0:
  addiu $a0, $a0, 1
  jr $s0

main:
  # j skips the next instruction
  li $a0, 1
  j j_done
  li $a0, 0              # skipped
j_done:
  jal print_int          # 1

  # jr to an address held in a register
  la $t0, jr_done
  li $a0, 1
  jr $t0
  li $a0, 0              # skipped
jr_done:
  jal print_int          # 1

  # jalr rd, rs: call through a register, return address in rd
  la $t0, add10
  li $a0, 5
  jalr $ra, $t0
  jal print_int          # 15
  la $t0, add1_s0
  li $a0, 7
  jalr $s0, $t0          # return address in $s0 instead of $ra
  jal print_int          # 8

  li $t1, -1

  # bgezal: call if >= 0
  li $a0, 20
  bgezal $zero, add10    # taken
  jal print_int          # 30
  li $a0, 40
  bgezal $t1, add10      # not taken
  jal print_int          # 40

  # bltzal: call if < 0
  li $a0, 50
  bltzal $t1, add10      # taken
  jal print_int          # 60
  li $a0, 70
  bltzal $zero, add10    # not taken
  jal print_int          # 70

  # bltzall: likely variant, same behavior here
  li $a0, 80
  bltzall $t1, add10     # taken
  jal print_int          # 90
  li $a0, 100
  bltzall $zero, add10   # not taken
  jal print_int          # 100

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
