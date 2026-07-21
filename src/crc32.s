    .intel_syntax noprefix
    .text

    .section .bss
    .align 32
table:
    .zero 1024
table_init:
    .byte 0

    .text

    .globl crc32_calculate
    .type crc32_calculate, @function
crc32_calculate:
    push    r12
    push    r13

    mov     r12, rdi         /* r12 = data                */
    mov     r13, rsi         /* r13 = size                */

    call    init_crc32_table

    lea     r10, [rip + table]
    mov     eax, 0xFFFFFFFF  /* crc = CRC32_INIT           */
    xor     rcx, rcx         /* i = 0                      */

.Lbyte_loop:
    cmp     rcx, r13
    jge     .Lbyte_loop_done

    movzx   edx, byte ptr [r12 + rcx]  /* edx = data[i]              */
    xor     dl, al                     /* dl  = data[i] ^ (byte)crc  */
    movzx   edx, dl                    /* zero-extend index to 0..255*/

    shr     eax, 8                     /* crc >>= 8                  */
    xor     eax, [r10 + rdx*4]         /* crc ^= table[index]        */

    inc     rcx
    jmp     .Lbyte_loop

.Lbyte_loop_done:
    not     eax              /* crc ^ CRC32_XOROUT (0xFFFFFFFF) */

    pop     r13
    pop     r12
    ret
    .size crc32_calculate, .-crc32_calculate

    .type init_crc32_table, @function
init_crc32_table:
    cmp     byte ptr [rip + table_init], 0
    jne     .Linit_done

    push    rbx
    push    r14
    push    r15

    xor     r14d, r14d        /* i = 0 */
.Ltable_loop:
    cmp     r14d, 256
    jge     .Ltable_loop_done

    mov     ebx, r14d          /* current_byte = i */
    xor     r15d, r15d         /* b = 0 */
.Lbit_loop:
    cmp     r15d, 8
    jge     .Lbit_loop_done

    test    ebx, 1
    jz      .Lshift_only

    shr     ebx, 1
    xor     ebx, 0xEDB88320    /* POLYNOMIAL */
    jmp     .Lbit_next
.Lshift_only:
    shr     ebx, 1
.Lbit_next:
    inc     r15d
    jmp     .Lbit_loop

.Lbit_loop_done:
    lea     rax, [rip + table]
    mov     [rax + r14*4], ebx
    inc     r14d
    jmp     .Ltable_loop

.Ltable_loop_done:
    mov     byte ptr [rip + table_init], 1

    pop     r15
    pop     r14
    pop     rbx

.Linit_done:
    ret
    .size init_crc32_table, .-init_crc32_table

    .section .note.GNU-stack,"",@progbits
