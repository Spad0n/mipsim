# Test: syscall 4 (print string). Expected: "Hello" followed by a newline
.data
msg: .asciiz "Hello\n"
.text
  li $v0, 4        # print_string
  la $a0, msg      # address of the string
  syscall
  li $v0, 10       # exit
  syscall
