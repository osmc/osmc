        .text
        .align  4,0x90
        .globl  _start
        .globl  _start1
_start:
_start1:
	mov     %ds, %bx
	mov     %bx, %es
	mov     %eax, %ebp

        .text
        .globl __start
        .set __start, (_start1)

	.align  4,0x90
	.globl  start1
start1:
	mov     %ebp, %ebx
	mov     $0,%ax
	mov     %ax,%fs
	mov     %ax,%gs

	pushl   %ebp
	xorl    %ebp,%ebp

	call    _load_linux
	hlt
