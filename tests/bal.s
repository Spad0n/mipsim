# Test: bal must return right after itself (ra = PC + 4). Expected: 111
.text
  bal f            # call f, ra = address of the next instruction
  li $v0, 1        # print_int
  li $a0, 111      # must be printed
  syscall
  li $v0, 10       # exit
  syscall
f:
  jr $ra           # return to the caller
