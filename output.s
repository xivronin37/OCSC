.data
fmt: .string "%d\n"
charFmt: .string "%c"
strFmt: .string "%s\n"
mapMissMsg: .string "Error: key not found in map\n"
mapOverflowMsg: .string "Error: map overflow\n"
.text
.global main
.def main; .scl 2; .type 32; .endef
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $128, %rsp
	jmp .L_skip_Lexer_advance
Lexer_advance:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
	movq %rcx, -8(%rbp)
	movq -8(%rbp), %r8
	movq 0(%r8), %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %r8
	movq -8(%r8), %rax
	negq %rax
	movq -16(%rbp), %rdx
	movq (%rdx, %rax, 8), %rax
	movq %rax, -24(%rbp)
	movq -8(%rbp), %r8
	movq -24(%r8), %rax
	pushq %rax
	movq $1, %rax
	movq %rax, %rbx
	popq %rax
	addq %rbx, %rax
	movq -8(%rbp), %r8
	movq %rax, -24(%r8)
	movq -8(%rbp), %r8
	movq -8(%r8), %rax
	pushq %rax
	movq $1, %rax
	movq %rax, %rbx
	popq %rax
	addq %rbx, %rax
	movq -8(%rbp), %r8
	movq %rax, -8(%r8)
	movq -24(%rbp), %rax
	leave
	ret
	leave
	ret
.L_skip_Lexer_advance:
	jmp .L_skip_Lexer_peek
Lexer_peek:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
	movq %rcx, -8(%rbp)
	movq -8(%rbp), %r8
	movq 0(%r8), %rax
	movq %rax, -16(%rbp)
	movq -8(%rbp), %r8
	movq -8(%r8), %rax
	pushq %rax
	movq -16(%rbp), %rax
	movq 8(%rax), %rax
	movq %rax, %rbx
	popq %rax
	cmpq %rbx, %rax
	setl %al
	movzbq %al, %rax
	cmpq $0, %rax
	je .L_else0
	movq -8(%rbp), %r8
	movq -8(%r8), %rax
	negq %rax
	movq -16(%rbp), %rdx
	movq (%rdx, %rax, 8), %rax
	leave
	ret
	jmp .L_done0
.L_else0:
	movq $0, %rax
	leave
	ret
	.L_done0:
	leave
	ret
.L_skip_Lexer_peek:
	movq $2, -8(%rbp)
	movq $104, %rax
	movq %rax, -16(%rbp)
	movq $105, %rax
	movq %rax, -24(%rbp)
	leaq -16(%rbp), %rax
	movq %rax, -32(%rbp)
	movq $0, %rax
	movq %rax, -40(%rbp)
	movq $1, %rax
	movq %rax, -48(%rbp)
	movq $0, %rax
	movq %rax, -56(%rbp)
	leaq -32(%rbp), %rcx
	subq $32, %rsp
	call Lexer_advance
	addq $32, %rsp
	movq %rax, %rdx
	leaq fmt(%rip), %rcx
	subq $32, %rsp
	call printf
	addq $32, %rsp
	leaq -32(%rbp), %rcx
	subq $32, %rsp
	call Lexer_advance
	addq $32, %rsp
	movq %rax, %rdx
	leaq fmt(%rip), %rcx
	subq $32, %rsp
	call printf
	addq $32, %rsp
	movq -40(%rbp), %rax
	movq %rax, %rdx
	leaq fmt(%rip), %rcx
	subq $32, %rsp
	call printf
	addq $32, %rsp
	movq -56(%rbp), %rax
	movq %rax, %rdx
	leaq fmt(%rip), %rcx
	subq $32, %rsp
	call printf
	addq $32, %rsp
	leave
	ret
runtime_error:
	subq $32, %rsp
	call printf
	movq $1, %rcx
	call exit
