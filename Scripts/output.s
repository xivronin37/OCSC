.data
fmt: .string "%d\n"
strFmt: .string "%s\n"
mapMissMsg: .string "Error: key not found in map\n"
mapOverflowMsg: .string "Error: map overflow\n"
.text
.global main
.def main; .scl 2; .type 32; .endef
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $0, %rsp
.data
printStr0: .string "foo"
.text
	leaq printStr0(%rip), %rdx
	leaq strFmt(%rip), %rcx
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
