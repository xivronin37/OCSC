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
	subq $32, %rsp
	jmp .L_skip_Player_takeDamage
Player_takeDamage:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
	movq %rcx, -8(%rbp)
	movq %rdx, -16(%rbp)
	movq -8(%rbp), %r8
	movq 0(%r8), %rax
	pushq %rax
	movq -16(%rbp), %rax
	movq %rax, %rbx
	popq %rax
	subq %rbx, %rax
	movq -8(%rbp), %r8
	movq %rax, 0(%r8)
	leave
	ret
.L_skip_Player_takeDamage:
	movq $100, %rax
	movq %rax, -8(%rbp)
	leaq -8(%rbp), %rcx
	movq $30, %rax
	movq %rax, %rdx
	subq $32, %rsp
	call Player_takeDamage
	addq $32, %rsp
	movq -8(%rbp), %rax
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
