# Intent:   a branch to itself (offset -1) must loop forever.
#           Not run by `xmake test` (no .expected file, a timeout is always a
#           failure): run it by hand and stop it with Ctrl-C.
# Expected: no output at all, 444 must NEVER be printed
.text
loop:
  b loop                 # infinite loop
  li $v0, 1              # print_int
  li $a0, 444            # must never be printed
  syscall
  li $v0, 10             # exit
  syscall
