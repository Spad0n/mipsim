# Test: a branch to itself must loop forever. Expected: timeout, 444 must NEVER be printed
# Not run by `xmake test` (no .expected file): check it by hand.
.text
loop: b loop       # infinite loop
  li $v0, 1        # print_int
  li $a0, 444      # must never be printed
  syscall
  li $v0, 10       # exit
  syscall
