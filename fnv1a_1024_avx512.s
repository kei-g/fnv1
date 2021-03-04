.section .text
	.global FNV1_BASIS1024
	.global fnv1_errno
	.global fnv1a_1024_avx512
	.global read

fnv1a_1024_avx512:             # rdi: fnv1
	push %rbp
	push %rbx
	push %rsi
	push %rdi
	push %r12
	push %r13
	push %r14
	push %r15
	mov %rdi, %rbp             # rbp: fnv1
							   #  1(%rbp): fnv1->fd
							   #  5(%rbp): fnv1->filename
							   # 13(%rbp): fnv1->function (== fnv1a_1024_avx512)
							   # 21(%rbp): fnv1->hash
							   # 29(%rbp): fnv1->progname
							   # 37(%rbp): fnv1->readbuf
							   # 45(%rbp): fnv1->readbufsize
							   # 53(%rbp): fnv1->work
	mov 37(%rbp), %rsi         # rsi: fnv1->readbuf
	mov 45(%rbp), %r14         # r14: fnv1->readbufsize
	mov 53(%rbp), %r15         # r15: work = fnv1->work

fnv1a_1024_avx512_load_basis:
	lea FNV1_BASIS1024, %rax
	vmovdqa64 (%rax), %zmm0
	vmovdqa64 64(%rax), %zmm1

fnv1a_1024_avx512_fetch:
	mov 1(%rbp), %rdi          # fnv1->fd
	mov %r14, %rdx             # fnv1->readbufsize
	call read                  # read(fnv1->fd, fnv1->readbuf, fnv1->readbufsize)
	test %rax, %rax
	js fnv1a_1024_avx512_failure
	jz fnv1a_1024_avx512_success
	xor %r14, %r14
	mov %rsi, %rbx
	mov %rax, %rcx
	cmp %rdx, %rax
	cmove %rax, %r14
	je fnv1a_1024_avx512_prepare

	add %rax, %rbx
	xor %al, %al
fnv1a_1024_avx512_bzero:
	mov %al, (%rbx)
	add $1, %bl
	jns fnv1a_1024_avx512_bzero
	add $127, %rcx
	mov $0x18d, %eax

fnv1a_1024_avx512_prepare:
	shr $7, %rcx

fnv1a_1024_avx512_loop_head:
fnv1a_1024_avx512_xor_hash:
	vpxorq (%rsi), %zmm0, %zmm2
	vpxorq 64(%rsi), %zmm1, %zmm3
	add $128, %rsi

fnv1a_1024_avx512_multiply_prime1024_hsw:
	vpxorq %xmm4, %xmm4, %xmm4
	vinserti64x2 $0, %xmm4, %zmm2, %zmm5
	vshuff64x2 $0x90, %zmm5, %zmm5, %zmm6
	vpslldq $5, %zmm2, %zmm7
	vpalignr $11, %zmm6, %zmm6, %zmm8
	vporq %zmm7, %zmm8, %zmm9
	vmovdqa64 %zmm9, 128(%r15) # hash dw[0-15] * FNV1_PRIME1024_HSW => hash[10-17]

fnv1a_1024_avx512_multiply_low:
	vmovd %eax, %xmm10         # FNV1_PRIME1024_LSW
	# XXX: TODO

fnv1a_1024_avx512_success:
	mov 21(%rbp), %rax
	vmovdqa64 %zmm0, (%rax)
	vmovdqa64 %zmm1, 64(%rax)
	xor %rax, %rax
	jmp fnv1a_1024_avx512_return

fnv1a_1024_avx512_failure:
	call fnv1_errno
	mov (%rax), %rax

fnv1a_1024_avx512_return:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rdi
	pop %rsi
	pop %rbx
	pop %rbp
	ret
