# Intent:   check movz (move if zero) and movn (move if not zero).
# Expected: 5, 1, 5, 1
.text
  li $t0, 5              # value to move
  li $t1, 0
  li $t2, 9

  li $a0, 1
  movz $a0, $t0, $t1     # $t1 == 0: moved
  jal print_int
  li $a0, 1
  movz $a0, $t0, $t2     # $t2 != 0: unchanged
  jal print_int
  li $a0, 1
  movn $a0, $t0, $t2     # $t2 != 0: moved
  jal print_int
  li $a0, 1
  movn $a0, $t0, $t1     # $t1 == 0: unchanged
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
