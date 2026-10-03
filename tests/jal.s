# Intent:   jal must jump to the function and set $ra to the address of the
#           next instruction (PC + 4), so that jr $ra comes back right after it.
# Expected: 222
.text
  jal f                  # call f, $ra = address of the next instruction
  li $v0, 1              # print_int
  li $a0, 222            # must be printed
  syscall
  li $a0, 10             # '\n'
  li $v0, 11
  syscall
  li $v0, 10             # exit
  syscall
f:
  jr $ra                 # return to the caller
