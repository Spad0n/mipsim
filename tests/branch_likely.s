# Intent:   check the "likely" branches beql, bnel, blezl, bgtzl, bltzl and
#           bgezl. The delay slot is not emulated, so they behave like the
#           normal branches: when not taken, the next instruction runs.
#           Each check prints 1 if the branch is taken, 0 otherwise:
#             li $a0, 1 / branch / li $a0, 0 / label: print
# Expected: 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0
.text
  li $t0, 0
  li $t1, 1
  li $t2, -1

  li $a0, 1
  beql $t1, $t1, beql_t
  li $a0, 0              # executed only if not taken
beql_t:
  jal print_int
  li $a0, 1
  beql $t0, $t1, beql_n
  li $a0, 0              # executed only if not taken
beql_n:
  jal print_int
  li $a0, 1
  bnel $t0, $t1, bnel_t
  li $a0, 0              # executed only if not taken
bnel_t:
  jal print_int
  li $a0, 1
  bnel $t1, $t1, bnel_n
  li $a0, 0              # executed only if not taken
bnel_n:
  jal print_int
  li $a0, 1
  blezl $t0, blezl_t
  li $a0, 0              # executed only if not taken
blezl_t:
  jal print_int
  li $a0, 1
  blezl $t1, blezl_n
  li $a0, 0              # executed only if not taken
blezl_n:
  jal print_int
  li $a0, 1
  bgtzl $t1, bgtzl_t
  li $a0, 0              # executed only if not taken
bgtzl_t:
  jal print_int
  li $a0, 1
  bgtzl $t0, bgtzl_n
  li $a0, 0              # executed only if not taken
bgtzl_n:
  jal print_int
  li $a0, 1
  bltzl $t2, bltzl_t
  li $a0, 0              # executed only if not taken
bltzl_t:
  jal print_int
  li $a0, 1
  bltzl $t0, bltzl_n
  li $a0, 0              # executed only if not taken
bltzl_n:
  jal print_int
  li $a0, 1
  bgezl $t0, bgezl_t
  li $a0, 0              # executed only if not taken
bgezl_t:
  jal print_int
  li $a0, 1
  bgezl $t2, bgezl_n
  li $a0, 0              # executed only if not taken
bgezl_n:
  jal print_int

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
