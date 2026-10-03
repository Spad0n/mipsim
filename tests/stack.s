# Intent:   $sp must be initialized to the top of memory, so that a value
#           can be pushed on the stack and popped back.
# Expected: 333
.text
  li $t0, 333
  addiu $sp, $sp, -4     # push one word (fails if $sp starts at 0)
  sw $t0, 0($sp)
  li $t0, 0              # clobber the register
  lw $a0, 0($sp)         # pop it back
  addiu $sp, $sp, 4
  jal print_int          # 333
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
