	.file	"solv2.c"
	.version	"01.01"
gcc2_compiled.:
.section	.rodata
	.align 4
.LC0:
	.long 0x9ee75616,0x3cd203af
.text
	.align 4
.globl solv
	.type	 solv,@function
solv:
	pushl %ebp
	movl %esp,%ebp
	subl $72,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	fldz
	pushl $8
	movl 16(%ebp),%edx
	pushl %edx
	fstpt -60(%ebp)
	call calloc
	movl %eax,-20(%ebp)
	movl $0,-4(%ebp)
	movl 8(%ebp),%ecx
	movl %ecx,-12(%ebp)
	movl %ecx,-16(%ebp)
	addl $8,%esp
	fldt -60(%ebp)
	movl 16(%ebp),%edi
	cmpl %edi,-4(%ebp)
	jge .L72
	leal 0(,%edi,8),%edx
	movl %edx,-24(%ebp)
	addl $8,%edx
	movl %edx,-32(%ebp)
	movl $0,-40(%ebp)
	movl 12(%ebp),%ecx
	movl %ecx,-44(%ebp)
	movl $0,-48(%ebp)
	.align 4
.L7:
	cmpl $0,-4(%ebp)
	je .L8
	movl $0,-64(%ebp)
	movl -20(%ebp),%edi
	movl %edi,-72(%ebp)
	movl -12(%ebp),%ebx
	movl 16(%ebp),%edx
	cmpl %edx,-64(%ebp)
	jge .L10
	.align 4
.L12:
	movl -72(%ebp),%ecx
	movl (%ebx),%eax
	movl %eax,(%ecx)
	movl 4(%ebx),%eax
	movl %eax,4(%ecx)
	addl $8,%ecx
	movl %ecx,-72(%ebp)
	incl -64(%ebp)
	addl -24(%ebp),%ebx
	movl 16(%ebp),%edi
	cmpl %edi,-64(%ebp)
	jl .L12
.L10:
	movl $1,-64(%ebp)
	movl 16(%ebp),%edx
	cmpl %edx,-64(%ebp)
	jge .L15
	movl -48(%ebp),%ecx
	movl %ecx,-28(%ebp)
	movl -20(%ebp),%edi
	addl $8,%edi
	movl %edi,-68(%ebp)
	movl %edx,-36(%ebp)
	.align 4
.L17:
	movl -64(%ebp),%edx
	movl %edx,-8(%ebp)
	movl -4(%ebp),%ecx
	cmpl %ecx,%edx
	jle .L18
	movl %ecx,-8(%ebp)
.L18:
	xorl %esi,%esi
	movl -36(%ebp),%edi
	movl -12(%ebp),%edx
	leal (%edx,%edi,8),%eax
	movl %eax,%ebx
	subl -28(%ebp),%ebx
	movl -20(%ebp),%ecx
        movl %ecx,%edi
	movl %ecx,-72(%ebp)
	movl -8(%ebp),%ecx
	fldz
	cmpl %esi,%ecx
	jle .L20
	.align 4
.L22:
	fldl (%ebx)
	fmull (%edi)
	faddp %st,%st(1)
	addl $8,%edi
	addl $8,%ebx
	incl %esi
	cmpl %esi,%ecx
	jg .L22
.L20:
	movl -72(%ebp),%ecx
	movl -68(%ebp),%edx
	fldl (%edx)
	fsubp %st,%st(1)
	fstpl (%edx)
	addl $8,%edx
	movl %edx,-68(%ebp)
	movl 16(%ebp),%ecx
	addl %ecx,-36(%ebp)
	incl -64(%ebp)
	cmpl %ecx,-64(%ebp)
	jl .L17
.L15:
	movl $0,-64(%ebp)
	movl -20(%ebp),%edi
	movl %edi,-72(%ebp)
	movl -12(%ebp),%ebx
	movl 16(%ebp),%edx
	cmpl %edx,-64(%ebp)
	jge .L8
	.align 4
.L28:
	movl -72(%ebp),%ecx
	movl (%ecx),%eax
	movl %eax,(%ebx)
	movl 4(%ecx),%eax
	movl %eax,4(%ebx)
	addl $8,%ecx
	movl %ecx,-72(%ebp)
	incl -64(%ebp)
	addl -24(%ebp),%ebx
	movl 16(%ebp),%edi
	cmpl %edi,-64(%ebp)
	jl .L28
.L8:
	movl -16(%ebp),%edx
	fldl (%edx)
	fabs
	movl -4(%ebp),%ecx
	movl %ecx,-8(%ebp)
	movl %ecx,%esi
	incl %esi
	movl %edx,-68(%ebp)
	cmpl %esi,16(%ebp)
	jle .L31
	.align 4
.L33:
	movl -24(%ebp),%edi
	addl %edi,-68(%ebp)
	movl -68(%ebp),%edx
	fldl (%edx)
	fabs
	fcom %st(1)
	fnstsw %ax
	andb $69,%ah
	jne .L73
	fstp %st(1)
	movl %esi,-8(%ebp)
	jmp .L32
	.align 4
.L73:
	fstp %st(0)
.L32:
	incl %esi
	cmpl %esi,16(%ebp)
	jg .L33
.L31:
	fld %st(0)
	fxch %st(2)
	fcom %st(1)
	fnstsw %ax
	andb $69,%ah
	jne .L74
	fstp %st(2)
	jmp .L36
	.align 4
.L74:
	fstp %st(0)
.L36:
	fldl .LC0
	fmul %st(2),%st
	fcompp
	fnstsw %ax
	andb $69,%ah
	jne .L38
	fstp %st(0)
	movl -20(%ebp),%ecx
	pushl %ecx
	call free
	movl $-1,%eax
	jmp .L71
	.align 4
.L38:
	movl -4(%ebp),%edi
	cmpl %edi,-8(%ebp)
	je .L39
	movl -44(%ebp),%edx
	fldl (%edx)
	movl -8(%ebp),%ecx
	movl 12(%ebp),%edi
	movl (%edi,%ecx,8),%eax
	movl %eax,(%edx)
	movl 4(%edi,%ecx,8),%eax
	movl %eax,4(%edx)
	fstpl (%edi,%ecx,8)
	xorl %esi,%esi
	movl -40(%ebp),%edx
	movl 8(%ebp),%ecx
	leal (%ecx,%edx,8),%ebx
	movl 16(%ebp),%eax
	imull -8(%ebp),%eax
	leal (%ecx,%eax,8),%eax
	movl %eax,-72(%ebp)
	cmpl %esi,16(%ebp)
	jle .L39
	.align 4
.L43:
	fldl (%ebx)
	movl -72(%ebp),%edi
	movl (%edi),%eax
	movl %eax,(%ebx)
	movl 4(%edi),%eax
	movl %eax,4(%ebx)
	addl $8,%ebx
	fstpl (%edi)
	addl $8,%edi
	movl %edi,-72(%ebp)
	incl %esi
	cmpl %esi,16(%ebp)
	jg .L43
.L39:
	movl -4(%ebp),%esi
	incl %esi
	movl -16(%ebp),%edx
	movl %edx,-68(%ebp)
	fld1
	fdivl (%edx)
	cmpl %esi,16(%ebp)
	jle .L75
	.align 4
.L48:
	movl -24(%ebp),%ecx
	addl %ecx,-68(%ebp)
	movl -68(%ebp),%edi
	fldl (%edi)
	fmul %st(1),%st
	fstpl (%edi)
	incl %esi
	cmpl %esi,16(%ebp)
	jg .L48
.L75:
	fstp %st(0)
	movl 16(%ebp),%edx
	addl %edx,-40(%ebp)
	addl $8,-44(%ebp)
	addl $8,-48(%ebp)
	incl -4(%ebp)
	addl $8,-12(%ebp)
	movl -32(%ebp),%ecx
	addl %ecx,-16(%ebp)
	cmpl %edx,-4(%ebp)
	jl .L7
.L72:
	fstp %st(0)
	movl $1,-4(%ebp)
	movl 12(%ebp),%edi
	addl $8,%edi
	movl %edi,-68(%ebp)
	movl 16(%ebp),%edx
	cmpl %edx,-4(%ebp)
	jge .L52
	movl 16(%ebp),%eax
	.align 4
.L54:
	xorl %esi,%esi
	movl 8(%ebp),%ecx
	leal (%ecx,%eax,8),%ebx
	movl 12(%ebp),%edi
	movl %edi,-72(%ebp)
	fldz
	cmpl %esi,-4(%ebp)
	jle .L56
	.align 4
.L58:
	fldl (%ebx)
	movl -72(%ebp),%edx
	fmull (%edx)
	faddp %st,%st(1)
	addl $8,%edx
	movl %edx,-72(%ebp)
	addl $8,%ebx
	incl %esi
	cmpl %esi,-4(%ebp)
	jg .L58
.L56:
	movl -68(%ebp),%ecx
	fldl (%ecx)
	fsubp %st,%st(1)
	fstpl (%ecx)
	addl $8,%ecx
	movl %ecx,-68(%ebp)
	addl 16(%ebp),%eax
	incl -4(%ebp)
	movl 16(%ebp),%edi
	cmpl %edi,-4(%ebp)
	jl .L54
.L52:
	movl 16(%ebp),%edx
	decl %edx
	movl %edx,-4(%ebp)
	addl $-8,-68(%ebp)
	movl 16(%ebp),%eax
	imull %eax,%eax
	movl 8(%ebp),%ecx
	leal -8(%ecx,%eax,8),%eax
	movl %eax,-16(%ebp)
	testl %edx,%edx
	jl .L62
	movl 16(%ebp),%edi
	leal 8(,%edi,8),%edi
	movl %edi,-64(%ebp)
	leal 0(,%edx,8),%eax
	.align 4
.L64:
	movl -4(%ebp),%esi
	incl %esi
	movl -16(%ebp),%ebx
	movl 12(%ebp),%edx
	addl %eax,%edx
	movl %edx,-72(%ebp)
	fldz
	cmpl %esi,16(%ebp)
	jle .L66
	.align 4
.L68:
	addl $8,%ebx
	addl $8,-72(%ebp)
	fldl (%ebx)
	movl -72(%ebp),%ecx
	fmull (%ecx)
	faddp %st,%st(1)
	incl %esi
	cmpl %esi,16(%ebp)
	jg .L68
.L66:
	movl -68(%ebp),%edi
	fldl (%edi)
	fsubp %st,%st(1)
	fstl (%edi)
	movl -16(%ebp),%edx
	fdivl (%edx)
	fstpl (%edi)
	addl $-8,%edi
	movl %edi,-68(%ebp)
	addl $-8,%eax
	movl -64(%ebp),%ecx
	subl %ecx,%edx
	movl %edx,-16(%ebp)
	decl -4(%ebp)
	jns .L64
.L62:
	movl -20(%ebp),%edi
	pushl %edi
	call free
	xorl %eax,%eax
.L71:
	leal -84(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe1:
	.size	 solv,.Lfe1-solv
	.ident	"GCC: (GNU) 2.7.2"
