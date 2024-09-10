global _start
_start:
    ;; let
    ;; div
    mov rax, 2
    push rax
    ;; sub
    ;; multi
    mov rax, 3
    push rax
    mov rax, 2
    push rax
    pop rax
    pop rbx
    mul rbx
    push rax
    ;; /multi
    mov rax, 10
    push rax
    pop rax
    pop rbx
    sub rax, rbx
    push rax
    ;; /sub
    pop rax
    pop rbx
    div rbx
    push rax
    ;; /div
    ;; /let
    ;; let
    mov rax, 7
    push rax
    ;; /let
    ;; if
    mov rax, 0
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax, 1
    push rax
    pop rax
    mov [rsp + 0], rax
    add rsp, 0
    jmp label1
label0:
    ;; elif
    mov rax, 0
    push rax
    pop rax
    test rax, rax
    jz label2
    mov rax, 2
    push rax
    pop rax
    mov [rsp + 0], rax
    add rsp, 0
    jmp label1
label2:
    mov rax, 3
    push rax
    pop rax
    mov [rsp + 0], rax
    add rsp, 0
    ;; /elif
label1:
    ;; /if
    ;; exit
    push QWORD [rsp + 0]
    mov rax, 60
    pop rdi
    syscall
    ;; /exit
    mov rax, 60
    mov rdi, 0
    syscall
