# Intent:   check the pseudo-instructions nop, move, li (every expansion:
#           addiu, ori, lui + ori, char literal), la, neg, negu and not.
# Expected: 5, 42, -1, 40000, 305419896, -100000, 65, 1234, -9, -9, -10
.data
val: .word 1234
.text
  li $a0, 5
  nop                    # does nothing
  jal print_int          # 5

  li $t0, 42
  move $a0, $t0
  jal print_int          # 42

  li $a0, -1             # addiu (signed 16 bits)
  jal print_int
  li $a0, 40000          # ori (unsigned 16 bits)
  jal print_int
  li $a0, 0x12345678     # lui + ori
  jal print_int
  li $a0, -100000        # lui + ori, negative
  jal print_int
  li $a0, 'A'            # char literal
  jal print_int

  la $t0, val            # lui + ori with the label address
  lw $a0, 0($t0)
  jal print_int          # 1234

  li $t0, 9
  neg $a0, $t0           # sub $a0, $zero, $t0
  jal print_int          # -9
  negu $a0, $t0          # subu $a0, $zero, $t0
  jal print_int          # -9
  not $a0, $t0           # nor $a0, $t0, $zero
  jal print_int          # -10

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
