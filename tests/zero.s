# Intent:   $zero always reads as 0, even after an instruction writes to it.
# Expected: 0
.text
  li $a0, 0
  addiu $zero, $zero, 5  # writing to $zero must have no effect
  addu $a0, $a0, $zero   # a0 = 0 + $zero
  jal print_int          # 0
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
