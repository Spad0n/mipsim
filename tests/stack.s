# Test: $sp must be initialized to the top of memory. Expected: 333
.text
  addiu $sp, $sp, -4   # push one word
  sw $ra, 0($sp)       # fails if $sp starts at 0
  li $v0, 1            # print_int
  li $a0, 333
  syscall
  li $v0, 10           # exit
  syscall
