# Intent:   check lwl, lwr, swl and swr (unaligned word access, big-endian).
#           lwl/swl handle the bytes from the address up to the end of the
#           word, lwr/swr the bytes from the start of the word up to the
#           address.
# Expected: 573785173, 287454020, 287454020, 1153158365, -1430533103,
#           2241348, 1426063360, 573785173, 573785173
.data
src:  .word 0x11223344, 0x55667788
dst:  .word 0, 0
full: .word 0, 0
.text
  la $t1, src

  # unaligned load of the word at src+1: bytes 22 33 44 55
  lwl $t0, 1($t1)
  lwr $t0, 4($t1)
  move $a0, $t0
  jal print_int          # 0x22334455

  lwl $a0, 0($t1)        # aligned lwl: whole word
  jal print_int          # 0x11223344
  lwr $a0, 3($t1)        # lwr on the last byte: whole word
  jal print_int          # 0x11223344

  li $t0, 0xAABBCCDD
  lwl $t0, 3($t1)        # only the top byte is replaced (by 0x44)
  move $a0, $t0
  jal print_int          # 0x44BBCCDD
  li $t0, 0xAABBCCDD
  lwr $t0, 0($t1)        # only the low byte is replaced (by 0x11)
  move $a0, $t0
  jal print_int          # 0xAABBCC11

  # unaligned store of 0x22334455 at dst+1
  la $t2, dst
  li $t0, 0x22334455
  swl $t0, 1($t2)        # bytes 1..3 of dst <- 22 33 44
  swr $t0, 4($t2)        # byte 0 of dst+4   <- 55
  lw $a0, 0($t2)
  jal print_int          # 0x00223344
  lw $a0, 4($t2)
  jal print_int          # 0x55000000

  # aligned swl / swr store the whole word
  la $t3, full
  li $t0, 0x22334455
  swl $t0, 0($t3)
  lw $a0, 0($t3)
  jal print_int          # 0x22334455
  swr $t0, 7($t3)
  lw $a0, 4($t3)
  jal print_int          # 0x22334455

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
