	.file	"intr.s"
	.text
	.align 1
	.section .text
	.global intr
	movi20 #0xfff88000, r15
	mov.l .Lentry, r0
	jmp @r0
	nop
intr:
	mov.l r15, @-r15
	mov.l r14, @-r15
	mov.l r13, @-r15
	mov.l r12, @-r15
	mov.l r11, @-r15
	mov.l r10, @-r15
	mov.l r9, @-r15
	mov.l r8, @-r15
	mov.l r7, @-r15
	mov.l r6, @-r15
	mov.l r5, @-r15
	mov.l r4, @-r15
	mov.l r3, @-r15
	mov.l r2, @-r15
	mov.l r1, @-r15
	mov.l r0, @-r15

	sts pr, r0
	mov.l r0, @-r15
	
	mov.l .Lintr, r0
	jsr/n @r0

	mov.l @r15+, r0
	lds r0, pr
	
	mov.l @r15+, r0
	mov.l @r15+, r1
	mov.l @r15+, r2
	mov.l @r15+, r3
	mov.l @r15+, r4
	mov.l @r15+, r5
	mov.l @r15+, r6
	mov.l @r15+, r7
	mov.l @r15+, r8
	mov.l @r15+, r9
	mov.l @r15+, r10
	mov.l @r15+, r11
	mov.l @r15+, r12
	mov.l @r15+, r13
	mov.l @r15+, r14
	add #4, r15
	rte
	nop

	.align 2
.Lintr:
	.long intrEntry
.Lentry:
	.long entry
.Libnr:
	.long 0xFFFD940e
