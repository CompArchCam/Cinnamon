	.text
	.file	"insbb.cpp"
	.globl	_Z6func_1m                      # -- Begin function _Z6func_1m
	.p2align	4, 0x90
	.type	_Z6func_1m,@function
_Z6func_1m:                             # @_Z6func_1m
# %bb.0:
	movq	%rdi, -8(%rsp)
	movq	inst_count, %rax
	addq	-8(%rsp), %rax
	movq	%rax, inst_count
	retq
.Lfunc_end0:
	.size	_Z6func_1m, .Lfunc_end0-_Z6func_1m
                                        # -- End function
	.type	inst_count,@object              # @inst_count
	.bss
	.globl	inst_count
	.p2align	3
inst_count:
	.quad	0                               # 0x0
	.size	inst_count, 8

	.ident	"Ubuntu clang version 11.0.0-2"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym inst_count
