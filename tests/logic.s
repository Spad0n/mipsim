# Intent:   check and, or, xor, nor, andi, ori, xori and lui. The logical
#           immediates are zero-extended, lui loads the upper 16 bits.
# Expected: 983055, 268374015, 267390960, -268374016, 255, 32768,
#           252706560, 305397760, -65536
.text
  li $t0, 0x0F0F00FF
  li $t1, 0x00FF0F0F

  and $a0, $t0, $t1      # 0x000F000F
  jal print_int
  or $a0, $t0, $t1       # 0x0FFF0FFF
  jal print_int
  xor $a0, $t0, $t1      # 0x0FF00FF0
  jal print_int
  nor $a0, $t0, $t1      # 0xF000F000
  jal print_int

  andi $a0, $t0, 0xFFFF  # 0x000000FF
  jal print_int
  ori $a0, $zero, 0x8000 # 0x00008000: zero-extended, not negative
  jal print_int
  xori $a0, $t0, 0xFFFF  # 0x0F0FFF00
  jal print_int

  lui $a0, 0x1234        # 0x12340000
  jal print_int
  lui $a0, 0xFFFF        # 0xFFFF0000
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
