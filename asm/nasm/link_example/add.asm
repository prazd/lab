global _add

section .text

_add:
    add edi, esi
    mov eax, edi
    ret
