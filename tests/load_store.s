# Intent:   check lw, lwu, lh, lhu, lb, lbu, sw, sh and sb. Memory is
#           big-endian: the byte at the lowest address is the most
#           significant one. Also checks a negative offset.
# Expected: 287454020, -2, -2, -32767, 32769, 32767, -128, 128, 127, 17,
#           68, 13124, 305419896, 305419896, 171, -1426063189, 305397760, 4660
.data
words: .word 0x11223344, -2
halfs: .half 0x8001, 0x7FFF
bytes: .byte 0x80, 0x7F, 1, 2
buf:   .space 16
.text
  la $t1, words
  lw $a0, 0($t1)         # 0x11223344
  jal print_int
  lw $a0, 4($t1)         # -2
  jal print_int
  lwu $a0, 4($t1)        # same as lw on a 32-bit machine
  jal print_int

  la $t1, halfs
  lh $a0, 0($t1)         # 0x8001 sign-extended
  jal print_int
  lhu $a0, 0($t1)        # 0x8001 zero-extended
  jal print_int
  lh $a0, 2($t1)         # 0x7FFF
  jal print_int

  la $t1, bytes
  lb $a0, 0($t1)         # 0x80 sign-extended
  jal print_int
  lbu $a0, 0($t1)        # 0x80 zero-extended
  jal print_int
  lb $a0, 1($t1)         # 0x7F
  jal print_int

  la $t1, words          # big-endian view of 0x11223344
  lb $a0, 0($t1)         # 0x11
  jal print_int
  lbu $a0, 3($t1)        # 0x44
  jal print_int
  lh $a0, 2($t1)         # 0x3344
  jal print_int

  la $t1, buf
  li $t0, 0x12345678
  sw $t0, 0($t1)
  lw $a0, 0($t1)         # 0x12345678
  jal print_int
  addiu $t2, $t1, 16
  lw $a0, -16($t2)       # negative offset, same word
  jal print_int

  li $t0, 0xAB
  sb $t0, 4($t1)         # most significant byte of buf+4
  sb $t0, 7($t1)         # least significant byte of buf+4
  lbu $a0, 4($t1)        # 0xAB
  jal print_int
  lw $a0, 4($t1)         # 0xAB0000AB
  jal print_int

  li $t0, 0x1234
  sh $t0, 8($t1)         # upper half of buf+8
  lw $a0, 8($t1)         # 0x12340000
  jal print_int
  lhu $a0, 8($t1)        # 0x1234
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
