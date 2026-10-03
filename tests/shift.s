# Intent:   check sll, srl, sra, sllv, srlv and srav. srl fills with zeros,
#           sra copies the sign bit, and the variable shifts only use the
#           low 5 bits of the shift register.
# Expected: 256, 134217729, -134217727, 4096, 8388608, -8388608
.text
  li $t0, 0x80000010     # sign bit set, so srl and sra differ

  sll $a0, $t0, 4        # 0x00000100: the top bits are shifted out
  jal print_int
  srl $a0, $t0, 4        # 0x08000001: zero fill
  jal print_int
  sra $a0, $t0, 4        # 0xF8000001: sign fill
  jal print_int

  li $t1, 40             # 40 & 31 = 8: only the low 5 bits count
  sllv $a0, $t0, $t1     # 0x00001000
  jal print_int
  srlv $a0, $t0, $t1     # 0x00800000
  jal print_int
  srav $a0, $t0, $t1     # 0xFF800000
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
