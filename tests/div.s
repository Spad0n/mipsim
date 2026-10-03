# Intent:   INT_MIN / -1 overflows in C (SIGFPE on x86). The VM must not crash
#           and must give the MIPS result: LO = 0x80000000, HI = 0.
# Expected: -2147483648, 0
.text
  li $t0, 0x80000000     # INT_MIN
  li $t1, -1
  div $t0, $t1
  mflo $a0
  jal print_int          # -2147483648
  mfhi $a0
  jal print_int          # 0
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
