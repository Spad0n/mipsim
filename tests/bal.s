# Intent:   bal (bgezal $zero) must return right after itself (ra = PC + 4),
#           both for a forward and a backward call (negative offset).
# Expected: 111, 112
.text
  j main
back:
  jr $ra                 # function placed before the call: backward branch
main:
  bal fwd                # forward call
  li $v0, 1              # print_int
  li $a0, 111            # must be printed
  syscall
  li $a0, 10             # '\n'
  li $v0, 11
  syscall

  bal back               # backward call
  li $v0, 1              # print_int
  li $a0, 112            # must be printed
  syscall
  li $a0, 10             # '\n'
  li $v0, 11
  syscall

  li $v0, 10             # exit
  syscall
fwd:
  jr $ra                 # return to the caller
