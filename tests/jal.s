# Reference test: jal already works. Expected: 222
.text
  jal f            # call f, ra = address of the next instruction
  li $v0, 1        # print_int
  li $a0, 222
  syscall
  li $v0, 10       # exit
  syscall
f:
  jr $ra           # return to the caller
