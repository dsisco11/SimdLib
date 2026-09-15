# Distinct executable/data COMDAT sections deliberately share their section names.
.section .text$case,"xr",discard,adversarial_primary
.globl adversarial_primary
.def adversarial_primary; .scl 2; .type 32; .endef
.globl adversarial_alias
adversarial_primary:
adversarial_alias:
 test %ecx,%ecx
 je adversarial_after
 ret
adversarial_after:
 call adversarial_opaque
 ret
.section .text$case,"xr",discard,adversarial_other
.globl adversarial_other
.def adversarial_other; .scl 2; .type 32; .endef
adversarial_other:
 nop
 ret
.section .rdata,"dr",discard,constant_one
.globl constant_one
constant_one:
 .quad 0x1122334455667788
.section .rdata,"dr",discard,constant_two
.globl constant_two
constant_two:
 .quad 0x8877665544332211
.section .text,"xr"
.globl adversarial_shared
.def adversarial_shared; .scl 2; .type 32; .endef
adversarial_shared:
 ret

