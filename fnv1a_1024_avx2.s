.section .text
	.global FNV1_BASIS1024
	.global fnv1_errno
	.global fnv1a_1024_avx2
	.global read

fnv1a_1024_avx2:               # rdi(fnv1)
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
							   # 13(%rbp): fnv1->function (== fnv1a_1024_avx2)
							   # 21(%rbp): fnv1->hash
							   # 29(%rbp): fnv1->progname
							   # 37(%rbp): fnv1->readbuf
							   # 45(%rbp): fnv1->readbufsize
							   # 53(%rbp): fnv1->work
	mov 37(%rbp), %rsi         # rsi: fnv1->readbuf
	mov 45(%rbp), %r14         # r14: fnv1->readbufsize
	mov 53(%rbp), %r15         # r15: work = fnv1->work

fnv1a_1024_avx2_load_basis:
	lea FNV1_BASIS1024, %rax
	vmovdqa (%rax), %ymm0
	vmovdqa 32(%rax), %ymm1
	vmovdqa 64(%rax), %ymm2
	vmovdqa 96(%rax), %ymm3

fnv1a_1024_avx2_fetch:
	mov 1(%rbp), %rdi          # fnv1->fd
	mov %r14, %rdx             # fnv1->readbufsize
	call read                  # read(fnv1->fd, values, fnv1->readbufsize)
	test %rax, %rax
	js fnv1a_1024_avx2_failure # if (rlen < 0)
	jz fnv1a_1024_avx2_success # if (rlen == 0)
	xor %r14, %r14
	mov %rsi, %rbx
	mov %rax, %rcx             # rlen
	cmp %rdx, %rax             # cmp fnv1->readbufsize, rlen
	cmove %rax, %r14
	je fnv1a_1024_avx2_prepare # if (rlen == fnv1->readbufsize)

	add %rax, %rbx
	xor %al, %al
fnv1a_1024_avx2_bzero:
	mov %al, (%rbx)
	add $1, %bl
	jns fnv1a_1024_avx2_bzero
	add $127, %rcx             # rlen += 127

fnv1a_1024_avx2_prepare:       # rcx = rlen, 0 < rcx && rcx <= fnv1->readbufsize
	shr $7, %rcx               # rcx: rlen / 128, then 0 < rcx && rcx <= fnv1->readbufsize / 128

fnv1a_1024_avx2_loop_head:
fnv1a_1024_avx2_xor_hash:
	vpxor (%rsi), %ymm0, %ymm4
	vpxor 32(%rsi), %ymm1, %ymm5
	vpxor 64(%rsi), %ymm2, %ymm6
	vpxor 96(%rsi), %ymm3, %ymm7
	add $128, %rsi
	mov $0x18d, %eax

fnv1a_1024_avx2_multiply_prime1024_hsw:
	vextracti128 $1, %ymm4, %xmm8
	vpslldq $5, %xmm4, %xmm9
	vpalignr $11, %xmm4, %xmm8, %xmm10
	vpalignr $11, %xmm8, %xmm5, %xmm11
	vmovdqa %xmm9, 128(%r15)   # hash[0-1] * FNV1_PRIME1024_HSW => hash[10-11]
	vmovdqa %xmm10, 144(%r15)  # hash[2-3] * FNV1_PRIME1024_HSW => hash[12-13]
	vmovdqa %xmm11, 160(%r15)  # hash[4-5] * FNV1_PRIME1024_HSW => hash[14-15]

fnv1a_1024_avx2_multiply_0x18d_low:
	vmovd %eax, %xmm12         # FNV1_PRIME1024_LSW
	vpshufd $0, %ymm12, %ymm13
	vpmuludq %ymm13, %ymm4, %ymm14
	vpmuludq %ymm13, %ymm5, %ymm15
	vpmuludq %ymm13, %ymm6, %ymm8
	vpmuludq %ymm13, %ymm7, %ymm12

fnv1a_1024_avx2_use_high_dword:
	vpshufd $0x31, %ymm0, %ymm0
	vpshufd $0x31, %ymm1, %ymm1
	vpshufd $0x31, %ymm2, %ymm2
	vpshufd $0x31, %ymm3, %ymm3

fnv1a_1024_avx2_store_low:
	vmovdqa %ymm14, (%r15)     # hash dw[0,2,4,6] * FNV1_PRIME1024_LSW => hash[0-1]
	vmovdqa %ymm15, 32(%r15)   # hash dw[8,10,12,14] * FNV1_PRIME1024_LSW => hash[2-3]
	vmovdqa %ymm8, 64(%r15)    # hash dw[16,18,20,22] * FNV1_PRIME1024_LSW => hash[4-5]
	vmovdqa %ymm12, 96(%r15)   # hash dw[24,26,28,30] * FNV1_PRIME1024_LSW => hash[6-7]

fnv1a_1024_avx2_multiply_0x18d_high:
	vmovd %eax, %xmm4
	vpshufd $0, %ymm4, %ymm5
	vpmuludq %ymm5, %ymm0, %ymm6
	vpmuludq %ymm5, %ymm1, %ymm7
	vpmuludq %ymm5, %ymm2, %ymm13
	vpmuludq %ymm5, %ymm3, %ymm9

fnv1a_1024_avx2_add_low:
	vextracti128 $1, %ymm6, %xmm10
	vextracti128 $1, %ymm7, %xmm11
	vmovq %xmm6, %rax
	vpextrq $1, %xmm6, %rdx
	vmovq %xmm10, %rbx
	vpextrq $1, %xmm10, %rdi
	vmovq %xmm7, %r8
	vpextrq $1, %xmm7, %r9
	vmovq %xmm11, %r10
	vpextrq $1, %xmm11, %r11
	vmovq %xmm13, %r12
	vpextrq $1, %xmm13, %r13
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
	vextracti128 $1, %ymm13, %xmm12
	vextracti128 $1, %ymm9, %xmm14
	vmovq %rax, %xmm0
	vpinsrq $1, %rdx, %xmm0, %xmm0
	vmovq %rbx, %xmm4
	vpinsrq $1, %rdi, %xmm4, %xmm4
	vinserti128 $1, %xmm4, %ymm0, %ymm0
	vmovq %r8, %xmm1
	vpinsrq $1, %r9, %xmm1, %xmm1
	vmovq %r10, %xmm5
	vpinsrq $1, %r11, %xmm5, %xmm5
	vinserti128 $1, %xmm5, %ymm1, %ymm1
	vmovq %r12, %xmm2
	vpinsrq $1, %r13, %xmm2, %xmm2

fnv1a_1024_avx2_add_middle:
	vmovq %xmm12, %rax
	vpextrq $1, %xmm12, %rdx
	vmovq %xmm9, %rbx
	vpextrq $1, %xmm9, %rdi
	vmovq %xmm14, %r8
	vpextrq $1, %xmm14, %r9
	adc 80(%r15), %rax
	adc 88(%r15), %rdx
	adc 96(%r15), %rbx
	adc 104(%r15), %rdi
	adc 112(%r15), %r8
	adc 120(%r15), %r9

fnv1a_1024_avx2_add_high:
	add 128(%r15), %rax
	adc 136(%r15), %rdx
	adc 144(%r15), %rbx
	adc 152(%r15), %rdi
	adc 160(%r15), %r8
	adc 168(%r15), %r9
	vmovq %rax, %xmm6
	vpinsrq $1, %rdx, %xmm6, %xmm6
	vinserti128 $1, %xmm6, %ymm2, %ymm2
	vmovq %rbx, %xmm3
	vpinsrq $1, %rdi, %xmm3, %xmm3
	vmovq %r8, %xmm7
	vpinsrq $1, %r9, %xmm7, %xmm7
	vinserti128 $1, %xmm7, %ymm3, %ymm3

fnv1a_1024_avx2_loop_tail:
	dec %rcx
	test %rcx, %rcx
	jnz fnv1a_1024_avx2_loop_head
	sub %r14, %rsi
	test %r14, %r14
	jnz fnv1a_1024_avx2_fetch

fnv1a_1024_avx2_success:
	mov 21(%rbp), %rax         # rax: hash = fnv1->hash
	vmovdqa %ymm0, (%rax)
	vmovdqa %ymm1, 32(%rax)
	vmovdqa %ymm2, 64(%rax)
	vmovdqa %ymm3, 96(%rax)
	xor %rax, %rax
	jmp fnv1a_1024_avx2_return

fnv1a_1024_avx2_failure:
	call fnv1_errno
	mov (%rax), %rax

fnv1a_1024_avx2_return:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rdi
	pop %rsi
	pop %rbx
	pop %rbp
	ret
