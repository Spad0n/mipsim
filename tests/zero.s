# Reference test: $zero always stays 0. Expected: 0
.text
  li $v0, 1              # print_int
  li $a0, 0
  addiu $zero, $zero, 5  # writing to $zero must have no effect
  addu $a0, $a0, $zero   # a0 = 0 + $zero
  syscall
  li $v0, 10             # exit
  syscall
