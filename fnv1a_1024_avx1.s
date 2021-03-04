.section .text
	.global FNV1_BASIS1024
	.global fnv1_errno
	.global fnv1a_1024_avx1
	.global read

fnv1a_1024_avx1:               # rdi(fnv1)
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
							   # 13(%rbp): fnv1->function (== fnv1a_1024_avx1)
							   # 21(%rbp): fnv1->hash
							   # 29(%rbp): fnv1->progname
							   # 37(%rbp): fnv1->readbuf
							   # 45(%rbp): fnv1->readbufsize
							   # 53(%rbp): fnv1->work
	mov 37(%rbp), %rsi         # rsi: fnv1->readbuf
	mov 45(%rbp), %r14         # r14: fnv1->readbufsize
	mov 53(%rbp), %r15         # r15: work = fnv1->work

fnv1a_1024_avx1_load_basis:
	lea FNV1_BASIS1024, %rax
	vmovdqa (%rax), %xmm0      # FNV1_BASIS1024[ 0- 1]
	vmovdqa 16(%rax), %xmm1    # FNV1_BASIS1024[ 2- 3]
	vmovdqa 32(%rax), %xmm2    # FNV1_BASIS1024[ 4- 5]
	vmovdqa 48(%rax), %xmm3    # FNV1_BASIS1024[ 6- 7]
	vmovdqa 64(%rax), %xmm4    # FNV1_BASIS1024[ 8- 9]
	vmovdqa 80(%rax), %xmm5    # FNV1_BASIS1024[10-11]
	vmovdqa 96(%rax), %xmm6    # FNV1_BASIS1024[12-13]
	vmovdqa 112(%rax), %xmm7   # FNV1_BASIS1024[14-15]

fnv1a_1024_avx1_fetch:
	mov 1(%rbp), %rdi          # fnv1->fd
	mov %r14, %rdx             # fnv1->readbufsize
	call read                  # read(fnv1->fd, values, fnv1->readbufsize)
	test %rax, %rax
	js fnv1a_1024_avx1_failure # if (rlen < 0)
	jz fnv1a_1024_avx1_success # if (rlen == 0)
	xor %r14, %r14
	mov %rsi, %rbx
	mov %rax, %rcx             # rlen
	cmp %rdx, %rax             # cmp fnv1->readbufsize, rlen
	cmove %rax, %r14
	je fnv1a_1024_avx1_prepare # if (rlen == fnv1->readbufsize)

	add %rax, %rbx
	xor %al, %al
fnv1a_1024_avx1_bzero:
	mov %al, (%rbx)
	add $1, %bl
	jns fnv1a_1024_avx1_bzero
	add $127, %rcx             # rlen += 127

fnv1a_1024_avx1_prepare:       # rcx = rlen, 0 < rcx && rcx <= fnv1->readbufsize
	shr $7, %rcx               # rcx: rlen / 128, then 0 < rcx && rcx <= fnv1->readbufsize / 128

fnv1a_1024_avx1_loop_head:
fnv1a_1024_avx1_xor_hash:
	vpxor (%rsi), %xmm0, %xmm8
	vpxor 16(%rsi), %xmm1, %xmm9
	vpxor 32(%rsi), %xmm2, %xmm10
	vpxor 48(%rsi), %xmm3, %xmm11
	vpxor 64(%rsi), %xmm4, %xmm12
	vpxor 80(%rsi), %xmm5, %xmm13
	vpxor 96(%rsi), %xmm6, %xmm14
	vpxor 112(%rsi), %xmm7, %xmm15
	add $128, %rsi
	mov $0x18d, %eax

fnv1a_1024_avx1_multiply_prime1024_hsw:
	vpslldq $5, %xmm8, %xmm0
	vpalignr $11, %xmm8, %xmm9, %xmm1
	vpalignr $11, %xmm9, %xmm10, %xmm2
	vmovdqa %xmm0, 128(%r15)   # hash dw[ 0- 2] * FNV1_PRIME1024_HSW => hash[10-11]
	vmovdqa %xmm1, 144(%r15)   # hash dw[ 4- 6] * FNV1_PRIME1024_HSW => hash[12-13]
	vmovdqa %xmm2, 160(%r15)   # hash dw[ 8-10] * FNV1_PRIME1024_HSW => hash[14-15]

fnv1a_1024_avx1_multiply_0x18d_low:
	vmovd %eax, %xmm3          # FNV1_PRIME1024_LSW
	vpshufd $0, %xmm3, %xmm4
	vpmuludq %xmm4, %xmm8, %xmm7
	vpmuludq %xmm4, %xmm9, %xmm6
	vpmuludq %xmm4, %xmm10, %xmm5
	vpmuludq %xmm4, %xmm11, %xmm3
	vpmuludq %xmm4, %xmm12, %xmm0
	vpmuludq %xmm4, %xmm13, %xmm1
	vpmuludq %xmm4, %xmm14, %xmm2
	vpmuludq %xmm4, %xmm15, %xmm4

fnv1a_1024_avx1_use_high_dword:
	vpshufd $0x31, %xmm8, %xmm8
	vpshufd $0x31, %xmm9, %xmm9
	vpshufd $0x31, %xmm10, %xmm10
	vpshufd $0x31, %xmm11, %xmm11
	vpshufd $0x31, %xmm12, %xmm12
	vpshufd $0x31, %xmm13, %xmm13
	vpshufd $0x31, %xmm14, %xmm14
	vpshufd $0x31, %xmm15, %xmm15

fnv1a_1024_avx1_store_low:
	vmovdqa %xmm7, (%r15)      # hash dw[ 0, 2] * FNV1_PRIME1024_LSW => hash[ 0- 1]
	vmovdqa %xmm6, 16(%r15)    # hash dw[ 4, 6] * FNV1_PRIME1024_LSW => hash[ 2- 3]
	vmovdqa %xmm5, 32(%r15)    # hash dw[ 8,10] * FNV1_PRIME1024_LSW => hash[ 4- 5]
	vmovdqa %xmm3, 48(%r15)    # hash dw[12,14] * FNV1_PRIME1024_LSW => hash[ 6- 7]
	vmovdqa %xmm0, 64(%r15)    # hash dw[16,18] * FNV1_PRIME1024_LSW => hash[ 8- 9]
	vmovdqa %xmm1, 80(%r15)    # hash dw[20,22] * FNV1_PRIME1024_LSW => hash[10-11]
	vmovdqa %xmm2, 96(%r15)    # hash dw[24,26] * FNV1_PRIME1024_LSW => hash[12-13]
	vmovdqa %xmm4, 112(%r15)   # hash dw[28,30] * FNV1_PRIME1024_LSW => hash[14-15]

fnv1a_1024_avx1_multiply_0x18d_high:
	vmovd %eax, %xmm7          # FNV1_PRIME1024_LSW
	vpshufd $0, %xmm7, %xmm6
	vpmuludq %xmm6, %xmm8, %xmm5
	vpmuludq %xmm6, %xmm9, %xmm3
	vpmuludq %xmm6, %xmm10, %xmm0
	vpmuludq %xmm6, %xmm11, %xmm1
	vpmuludq %xmm6, %xmm12, %xmm2
	vpmuludq %xmm6, %xmm13, %xmm4
	vpmuludq %xmm6, %xmm14, %xmm7
	vpmuludq %xmm6, %xmm15, %xmm6

fnv1a_1024_avx1_align:
	vpslldq $4, %xmm5, %xmm8   # (hash dw[ 1, 3] * FNV1_PRIME1024_LSW) << 32 => hash[ 0- 1]
	vpalignr $12, %xmm5, %xmm3, %xmm9
	vpalignr $12, %xmm3, %xmm0, %xmm10
	vpalignr $12, %xmm0, %xmm1, %xmm11
	vpalignr $12, %xmm1, %xmm2, %xmm12
	vpalignr $12, %xmm2, %xmm4, %xmm13
	vpalignr $12, %xmm4, %xmm7, %xmm14
	vpalignr $12, %xmm7, %xmm6, %xmm15

fnv1a_1024_avx1_add_low:
	vmovq %xmm8, %rax
	vpextrq $1, %xmm8, %rdx
	vmovq %xmm9, %rbx
	vpextrq $1, %xmm9, %rdi
	vmovq %xmm10, %r8
	vpextrq $1, %xmm10, %r9
	vmovq %xmm11, %r10
	vpextrq $1, %xmm11, %r11
	vmovq %xmm12, %r12
	vpextrq $1, %xmm12, %r13
	add (%r15), %rax
	adc 8(%r15), %rdx
	adc 16(%r15), %rbx
	adc 24(%r15), %rdi
	adc 32(%r15), %r8
	adc 40(%r15), %r9
	adc 48(%r15), %r10
	adc 56(%r15), %r11
	adc 64(%r15), %r12
	adc 72(%r15), %r13
	vmovq %rax, %xmm0
	vpinsrq $1, %rdx, %xmm0, %xmm0
	vmovq %rbx, %xmm1
	vpinsrq $1, %rdi, %xmm1, %xmm1
	vmovq %r8, %xmm2
	vpinsrq $1, %r9, %xmm2, %xmm2
	vmovq %r10, %xmm3
	vpinsrq $1, %r11, %xmm3, %xmm3
	vmovq %r12, %xmm4
	vpinsrq $1, %r13, %xmm4, %xmm4

fnv1a_1024_avx1_add_middle:
	vmovq %xmm13, %rax
	vpextrq $1, %xmm13, %rdx
	vmovq %xmm14, %rbx
	vpextrq $1, %xmm14, %rdi
	vmovq %xmm15, %r8
	vpextrq $1, %xmm15, %r9
	adc 80(%r15), %rax
	adc 88(%r15), %rdx
	adc 96(%r15), %rbx
	adc 104(%r15), %rdi
	adc 112(%r15), %r8
	adc 120(%r15), %r9

fnv1a_1024_avx1_add_high:
	add 128(%r15), %rax
	adc 136(%r15), %rdx
	adc 144(%r15), %rbx
	adc 152(%r15), %rdi
	adc 160(%r15), %r8
	adc 168(%r15), %r9
	vmovq %rax, %xmm5
	vpinsrq $1, %rdx, %xmm5, %xmm5
	vmovq %rbx, %xmm6
	vpinsrq $1, %rdi, %xmm6, %xmm6
	vmovq %r8, %xmm7
	vpinsrq $1, %r9, %xmm7, %xmm7

fnv1a_1024_avx1_loop_tail:
	dec %rcx
	test %rcx, %rcx
	jnz fnv1a_1024_avx1_loop_head
	sub %r14, %rsi
	test %r14, %r14
	jnz fnv1a_1024_avx1_fetch

fnv1a_1024_avx1_success:
	mov 21(%rbp), %rax         # rax: hash = fnv1->hash
	vmovdqa %xmm0, (%rax)
	vmovdqa %xmm1, 16(%rax)
	vmovdqa %xmm2, 32(%rax)
	vmovdqa %xmm3, 48(%rax)
	vmovdqa %xmm4, 64(%rax)
	vmovdqa %xmm5, 80(%rax)
	vmovdqa %xmm6, 96(%rax)
	vmovdqa %xmm7, 112(%rax)
	xor %rax, %rax
	jmp fnv1a_1024_avx1_return

fnv1a_1024_avx1_failure:
	call fnv1_errno
	mov (%rax), %rax

fnv1a_1024_avx1_return:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rdi
	pop %rsi
	pop %rbx
	pop %rbp
	ret
