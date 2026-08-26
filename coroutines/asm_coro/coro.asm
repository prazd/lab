global _store_rip
global _load_rip

section .text

_store_rip:
    push rax
    mov rax, QWORD[rsp + 8]
    add rax, 2
    mov QWORD[rdi], rax
    pop rax
    ret

_load_rip:
    push rax
    mov rax, QWORD[rdi]
    mov QWORD[rsp+8], rax
    mov rdi, QWORD[rsp+16]
    pop rax
    ret