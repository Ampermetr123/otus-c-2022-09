    bits 64
    extern malloc, free, puts, printf, fflush, abort
    global main

    section   .data
empty_str: db 0x0
int_format: db "%ld ", 0x0
data: dq 4, 8, 15, 16, 23, 42 ; uint64_t
data_length: equ ($-data) / 8 ; sizeof(data)

    section   .text
;;; print_int proc
print_int:
    ; rdi указатель на int
    mov r14, rsp ; align stack
    and rsp, -16 ;

    mov rsi, [rdi] 
    mov rdi, int_format
    xor rax, rax
    call printf

    xor rdi, rdi
    call fflush

    mov rsp, r14
    ret

free_cb:
   ; rdi указатель на структуру
    mov r14, rsp ; align stack
    and rsp, -16 ;
    call free 
    mov rsp, r14
    ret

;;; p proc
p:
    mov rax, rdi
    and rax, 1
    ret

;;; add_element proc
add_element: ; a_ (prev) , b_(value)
    push rbp ; save stack
    push rbx ; save bx, we use it in such_function

    mov r14, rsp ; align stack
    and rsp, -16 ;

    mov rbp, rdi  ; rbp = a_
    mov rbx, rsi ;  rbx = b_ 

    mov rdi, 16 ; выделили 16 байт
    call malloc
    test rax, rax
    jz abort

    mov [rax], rbp  ; *ptr[0] = a_
    mov [rax + 8], rbx ; *ptr[1] = b_ 

    ; restore stack after align
    mov rsp, r14
    pop rbx
    pop rbp
    ret

;;; m proc  walk over list using recucrsion
m:
    ; rdi - указатель на элемент списка
    ; rsi - функция
    test rdi, rdi
    jz outm
    push rsi
    mov r13, [rdi + 8] ; указатель на следующий 
    call rsi
    pop rsi
    mov rdi, r13
    call m
outm:
    ret

;;; f proc
f:
    ; rdi = указатель на элемент списка
    ; rsi = изнчаельно 0  (новый список)
    ; rdx = функция f(rdi) условие добавления в новый список
    mov rax, rsi

    test rdi, rdi
    jz outf ; выход, если rdi==0

    push rbx ; store what using
    push r12
    push r13

    mov rbx, rdi ; указатель списка
    mov r12, rsi ; значение изначально 0
    mov r13, rdx ; функция f(rdi)

    mov rdi, [rdi] ; берем значение
    call rdx ; вызов f для значения
    test rax, rax ; елси 0, - для p() - если значение нечетное
    jz z

    ; добавляетя значение *rbx (= значение элемета списка) в список rsi
    mov rdi, [rbx] 
    mov rsi, r12
    call add_element
    mov rsi, rax
    jmp ff

z:
    mov rsi, r12

ff: ; реуксисный вызов f() для следующего элемента списка
    mov rdi, [rbx + 8]
    mov rdx, r13
    call f
    pop r13
    pop r12
    pop rbx

outf:
    ret

;;; main proc
main:
    push rbx ; вызов функции
    xor rax, rax ; a=0
    mov rbx, data_length ; j=data_length
adding_loop:
    mov rdi, [data - 8 + rbx * 8] ; b=data[j-1]
    mov rsi, rax ; a_=a, b_ = b;  add_elemet(a_,b_);
    call add_element ; out rax - указатeль на структуру
    dec rbx ; j-=1
    jnz adding_loop ; until (j==0)

    mov rbx, rax ; заголовок списка

    ; вывод первого списка
    mov rdi, rbx 
    mov rsi, print_int
    call m
    mov rdi, empty_str
    call puts
    
   
    mov rdx, p
    xor rsi, rsi ;=0
    mov rdi, rbx ; заголовок входного списка
    call f
    
    ; вывод второго списка
    push rax;
    mov rdi, rax
    mov rsi, print_int
    call m
    mov rdi, empty_str
    call puts
    pop rax;

    ;удаление второго списка
    mov rdi, rax 
    mov rsi, free_cb
    call m
   
    ;удаление первого списка
    mov rdi, rbx 
    mov rsi, free_cb
    call m

    pop rbx
    xor rax, rax
    ret
