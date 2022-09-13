section .text
		align 4
global FixedMul
FixedMul:
	mov		eax, [esp+4]
	imul	dword [esp+8]
	shrd	eax, edx, 16
	ret

		align 4
global FixedDiv
FixedDiv:
	mov		eax, [esp+4]
	cdq
	shld edx, eax, 16
	sal eax, 16
	idiv dword[esp+8]
	ret
