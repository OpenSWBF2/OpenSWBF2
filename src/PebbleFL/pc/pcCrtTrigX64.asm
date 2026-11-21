EXTERN __imp_sin:QWORD
EXTERN __imp_cos:QWORD
EXTERN __imp_tan:QWORD
EXTERN __imp__errno:QWORD

.const

ALIGN 8
PcCrtTrigTwo55 QWORD 4360000000000000h
PcCrtTrigTwoM55 QWORD 3C80000000000000h
PcCrtTrigBelowOne QWORD 3FEFFFFFFFFFFFFFh
PcCrtTrigOne QWORD 3FF0000000000000h
PcCrtTrigCw WORD 027Fh
PcCrtTrigReduce BYTE 035h, 0C2h, 068h, 021h, 0A2h, 0DAh, 00Fh, 0C9h, 03Eh, 040h
PcCrtTrigIndefinite BYTE 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0C0h, 0FFh, 0FFh

.code

PcCrtTrigDomain PROC FRAME
    sub rsp, 40
    .allocstack 40
    .endprolog
    movsd qword ptr [rsp+32], xmm0
    call qword ptr [__imp__errno]
    mov dword ptr [rax], 21h
    movsd xmm0, qword ptr [rsp+32]
    add rsp, 40
    ret
PcCrtTrigDomain ENDP

PcCrtTrigLoad MACRO special
    movsd qword ptr [rsp+8], xmm0
    mov eax, dword ptr [rsp+12]
    and eax, 7FF00000h
    cmp eax, 7FF00000h
    je special
    fld qword ptr [rsp+8]
ENDM

PcCrtTrigSpecial MACRO store
    LOCAL domain
    mov eax, dword ptr [rsp+12]
    or eax, 7FFF0000h
    mov dword ptr [rsp+22], eax
    mov eax, dword ptr [rsp+12]
    mov ecx, dword ptr [rsp+8]
    shld eax, ecx, 0Bh
    shl ecx, 0Bh
    mov dword ptr [rsp+20], eax
    mov dword ptr [rsp+16], ecx
    fld tbyte ptr [rsp+16]
    fwait
    fnstcw word ptr [rsp+28]
    mov eax, dword ptr [rsp+12]
    test eax, 0FFFFFh
    jne store
    cmp dword ptr [rsp+8], 0
    jne store
    fstp st(0)
    fld tbyte ptr [PcCrtTrigIndefinite]
    fstp qword ptr [rsp+32]
    fld qword ptr [rsp+32]
    cmp word ptr [rsp+28], 027Fh
    je domain
    fldcw word ptr [rsp+28]
domain:
    fstp qword ptr [rsp+8]
    movsd xmm0, qword ptr [rsp+8]
    jmp PcCrtTrigDomain
ENDM

PcCrtTrigFinish MACRO store
    LOCAL restore
    mov ax, word ptr [rsp+28]
    cmp ax, 027Fh
    je store
    and ax, 20h
    je restore
    fwait
    fnstsw ax
    and ax, 20h
    je restore
    fstp qword ptr [rsp+32]
    fld qword ptr [rsp+32]
restore:
    fldcw word ptr [rsp+28]
store:
    fstp qword ptr [rsp+8]
    movsd xmm0, qword ptr [rsp+8]
    ret
ENDM

PcCrtTrigGate MACRO x87
    stmxcsr dword ptr [rsp+8]
    mov eax, dword ptr [rsp+8]
    and eax, 7F80h
    cmp eax, 1F80h
    jne x87
    fnstcw word ptr [rsp+8]
    mov ax, word ptr [rsp+8]
    and ax, 7Fh
    cmp ax, 7Fh
    jne x87
ENDM

PcCrtTrigEnter MACRO
    fwait
    fnstcw word ptr [rsp+28]
    cmp word ptr [rsp+28], 027Fh
    je @F
    fldcw word ptr [PcCrtTrigCw]
@@:
ENDM

PcCrtSinX64 PROC
    PcCrtTrigGate SinX87
    pextrw eax, xmm0, 3
    and ax, 7FFFh
    sub ax, 3030h
    cmp ax, 10C5h
    ja SinOut
    jmp qword ptr [__imp_sin]
SinOut:
    jg SinX87
    shr ax, 4
    cmp ax, 0CFDh
    jne SinTiny
    mulsd xmm0, qword ptr [PcCrtTrigBelowOne]
    ret
SinTiny:
    movlpd xmm3, qword ptr [PcCrtTrigTwo55]
    mulsd xmm3, xmm0
    subsd xmm3, xmm0
    mulsd xmm3, qword ptr [PcCrtTrigTwoM55]
    movapd xmm0, xmm3
    ret
SinX87:
    PcCrtTrigLoad SinSpecial
    PcCrtTrigEnter
    fsin
    fwait
    fnstsw ax
    sahf
    jp SinReduce
SinDone:
    PcCrtTrigFinish SinStore
SinReduce:
    fld tbyte ptr [PcCrtTrigReduce]
    fxch st(1)
SinPrem:
    fprem1
    fwait
    fnstsw ax
    sahf
    jp SinPrem
    fstp st(1)
    fsin
    jmp SinDone
SinSpecial:
    PcCrtTrigSpecial SinStore
PcCrtSinX64 ENDP

PcCrtCosX64 PROC
    PcCrtTrigGate CosX87
    pextrw eax, xmm0, 3
    and ax, 7FFFh
    sub ax, 3030h
    cmp ax, 10C5h
    ja CosOut
    jmp qword ptr [__imp_cos]
CosOut:
    jg CosX87
    pextrw eax, xmm0, 3
    and ax, 7FFFh
    pinsrw xmm0, eax, 3
    movlpd xmm1, qword ptr [PcCrtTrigOne]
    subsd xmm1, xmm0
    movapd xmm0, xmm1
    ret
CosX87:
    PcCrtTrigLoad CosSpecial
    PcCrtTrigEnter
    fcos
    fwait
    fnstsw ax
    sahf
    jp CosReduce
CosDone:
    PcCrtTrigFinish CosStore
CosReduce:
    fld tbyte ptr [PcCrtTrigReduce]
    fxch st(1)
CosPrem:
    fprem1
    fwait
    fnstsw ax
    sahf
    jp CosPrem
    fstp st(1)
    fcos
    jmp CosDone
CosSpecial:
    PcCrtTrigSpecial CosStore
PcCrtCosX64 ENDP

PcCrtTanX64 PROC
    PcCrtTrigGate TanX87
    pextrw eax, xmm0, 3
    and ax, 7FFFh
    sub ax, 3820h
    cmp ax, 8A8h
    ja TanOut
    jmp qword ptr [__imp_tan]
TanOut:
    jg TanX87
    shr ax, 4
    cmp ax, 0C7Eh
    jne TanTiny
    movsd xmm3, xmm0
    mulsd xmm3, qword ptr [PcCrtTrigTwoM55]
TanTiny:
    movlpd xmm3, qword ptr [PcCrtTrigTwo55]
    mulsd xmm3, xmm0
    addsd xmm3, xmm0
    mulsd xmm3, qword ptr [PcCrtTrigTwoM55]
    movapd xmm0, xmm3
    ret
TanX87:
    PcCrtTrigLoad TanSpecial
    PcCrtTrigEnter
    fptan
    fwait
    fnstsw ax
    sahf
    jp TanReduce
    fstp st(0)
TanDone:
    PcCrtTrigFinish TanStore
TanReduce:
    fld tbyte ptr [PcCrtTrigReduce]
    fxch st(1)
TanPrem:
    fprem1
    fwait
    fnstsw ax
    sahf
    jp TanPrem
    fstp st(1)
    fptan
    fstp st(0)
    jmp TanDone
TanSpecial:
    PcCrtTrigSpecial TanStore
PcCrtTanX64 ENDP

END
