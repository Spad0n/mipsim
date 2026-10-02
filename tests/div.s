# Test: INT_MIN / -1 must not crash the host. Expected: clean exit through syscall 10
.text
  li $t0, 0x80000000   # INT_MIN
  li $t1, -1
  div $t0, $t1         # overflows in C, must give LO = 0x80000000, HI = 0
  li $v0, 10           # exit
  syscall
