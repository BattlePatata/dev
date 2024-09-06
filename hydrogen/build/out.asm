global _start
_start:
    mov rax, 34
    push rax
    mov rax, 35
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    mov rax, 37
    push rax
    push QWORD [rsp + 8]

    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
