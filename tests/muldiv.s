# Intent:   check mult, multu, div, divu and the HI/LO registers (mfhi,
#           mflo, mthi, mtlo).
# Expected: -24, -1, -24, 3, 0, 1, -3, -1, 2147483644, 1, 123, 456
.text
  li $t0, -6
  li $t1, 4

  mult $t0, $t1          # -24: LO = 0xFFFFFFE8, HI = 0xFFFFFFFF
  mflo $a0
  jal print_int
  mfhi $a0
  jal print_int

  multu $t0, $t1         # 0xFFFFFFFA * 4 = 0x3_FFFFFFE8
  mflo $a0
  jal print_int
  mfhi $a0
  jal print_int

  li $t2, 0x10000
  mult $t2, $t2          # 2^32: LO = 0, HI = 1
  mflo $a0
  jal print_int
  mfhi $a0
  jal print_int

  li $t0, -7
  li $t1, 2
  div $t0, $t1           # rounds toward zero: LO = -3, HI = -1
  mflo $a0
  jal print_int
  mfhi $a0
  jal print_int

  divu $t0, $t1          # 0xFFFFFFF9 / 2 (unsigned)
  mflo $a0
  jal print_int
  mfhi $a0
  jal print_int

  li $t0, 123
  mthi $t0
  li $t0, 456
  mtlo $t0
  mfhi $a0               # 123
  jal print_int
  mflo $a0               # 456
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
