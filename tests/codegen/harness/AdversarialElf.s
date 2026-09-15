# Real ELF assembly fixtures exercise symbol and range selection independently of C++ optimization.
.text
.globl adversarial_primary
.type adversarial_primary,@function
adversarial_primary:
 test %edi,%edi
 je adversarial_after
 ret
adversarial_after:
 call adversarial_opaque
 ret
.size adversarial_primary,.-adversarial_primary
.globl adversarial_padding
.type adversarial_padding,@function
adversarial_padding:
 ret
.size adversarial_padding,.-adversarial_padding
 .p2align 4,0x90
.globl adversarial_next
.type adversarial_next,@function
adversarial_next:
 ret
.size adversarial_next,.-adversarial_next
.globl adversarial_empty
.type adversarial_empty,@function
adversarial_empty:
.size adversarial_empty,0
.section .text.one,"ax",@progbits
.type duplicate,@function
duplicate:
 ret
.size duplicate,.-duplicate
.section .text.two,"ax",@progbits
.type duplicate_two,@function
duplicate_two:
 nop
 ret
.size duplicate_two,.-duplicate_two
.section .rodata,"a",@progbits
.globl constant_one
constant_one:
 .quad 0x1122334455667788
.section .text.malformed,"ax",@progbits
.globl adversarial_decode
.type adversarial_decode,@function
adversarial_decode:
 .byte 0x0f
.size adversarial_decode,.-adversarial_decode
