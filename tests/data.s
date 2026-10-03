# Intent:   check the data directives .word (numbers and label), .half,
#           .byte, .ascii (no terminator), .asciiz, .space (zero-filled) and
#           .align 2 (next address multiple of 4).
# Expected: 100, -100, 1000, -2, 7, -7, 65, 66, 67, "CD", 0, 0, 0, 77, 0
.data
wd:    .word 100, -100
hf:    .half 1000, -2
bt:    .byte 7, -7
s1:    .ascii "AB"       # no NUL: directly followed by s2
s2:    .asciiz "CD"
gap:   .space 3
       .align 2
al:    .word 77
ptr:   .word wd          # holds the address of wd
.text
  la $t0, wd
  lw $a0, 0($t0)
  jal print_int          # 100
  lw $a0, 4($t0)
  jal print_int          # -100

  la $t0, hf
  lh $a0, 0($t0)
  jal print_int          # 1000
  lh $a0, 2($t0)
  jal print_int          # -2

  la $t0, bt
  lb $a0, 0($t0)
  jal print_int          # 7
  lb $a0, 1($t0)
  jal print_int          # -7

  la $t0, s1
  lbu $a0, 0($t0)
  jal print_int          # 'A'
  lbu $a0, 1($t0)
  jal print_int          # 'B'
  lbu $a0, 2($t0)
  jal print_int          # 'C': first byte of s2

  la $a0, s2
  li $v0, 4              # print_string
  syscall                # "CD"
  li $a0, 10             # '\n'
  li $v0, 11
  syscall
  la $t0, s2
  lbu $a0, 2($t0)
  jal print_int          # NUL terminator: 0

  la $t0, gap
  lbu $a0, 0($t0)
  jal print_int          # 0

  la $t0, al
  andi $a0, $t0, 3       # aligned on 4 bytes
  jal print_int          # 0
  lw $a0, 0($t0)
  jal print_int          # 77

  la $t0, ptr
  lw $t1, 0($t0)         # address stored by .word wd
  la $t2, wd
  subu $a0, $t1, $t2
  jal print_int          # 0: same address

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
