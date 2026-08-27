.data
fmt: .string "%d\n"
.text
.global main
.def main; .scl 2; .type 32; .endef
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
	movq $0, %rax
	movq %rax, -8(%rbp)
	movq $1, %rax
	movq %rax, -16(%rbp)
	movq $2, %rax
	movq %rax, -24(%rbp)
	movq -24(%rbp), %rax
	movq %rax, %rdx
	leaq fmt(%rip), %rcx
	subq $32, %rsp
	call printf
	addq $32, %rsp
	leave
	ret
