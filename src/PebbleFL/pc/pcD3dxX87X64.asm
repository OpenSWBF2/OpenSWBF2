EXTERN PcD3dxVec3Normalize:PROC

.const

PcD3dxX87Cw WORD 027Fh

.code

PcD3dxMatrixRotationX PROC
    movss dword ptr [rsp+8], xmm1
    fnstcw word ptr [rsp+16]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [PcD3dxX87Cw]
@@:
    fld dword ptr [rsp+8]
    fsincos
    fstp dword ptr [rsp+24]
    fstp dword ptr [rsp+28]
    fld1
    fst dword ptr [rcx]
    fldz
    fst dword ptr [rcx+4h]
    fst dword ptr [rcx+8h]
    fst dword ptr [rcx+0Ch]
    fst dword ptr [rcx+10h]
    fld dword ptr [rsp+24]
    fst dword ptr [rcx+14h]
    fld dword ptr [rsp+28]
    fst dword ptr [rcx+18h]
    fxch st(2)
    fst dword ptr [rcx+1Ch]
    fst dword ptr [rcx+20h]
    fxch st(2)
    fchs
    fstp dword ptr [rcx+24h]
    fstp dword ptr [rcx+28h]
    fst dword ptr [rcx+2Ch]
    fst dword ptr [rcx+30h]
    fst dword ptr [rcx+34h]
    fstp dword ptr [rcx+38h]
    fstp dword ptr [rcx+3Ch]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [rsp+16]
@@:
    mov rax, rcx
    ret
PcD3dxMatrixRotationX ENDP

PcD3dxMatrixRotationY PROC
    movss dword ptr [rsp+8], xmm1
    fnstcw word ptr [rsp+16]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [PcD3dxX87Cw]
@@:
    fld dword ptr [rsp+8]
    fsincos
    fstp dword ptr [rsp+24]
    fstp dword ptr [rsp+28]
    fld dword ptr [rsp+24]
    fst dword ptr [rcx]
    fldz
    fst dword ptr [rcx+4h]
    fld dword ptr [rsp+28]
    fld st(0)
    fchs
    fstp dword ptr [rcx+8h]
    fxch st(1)
    fst dword ptr [rcx+0Ch]
    fst dword ptr [rcx+10h]
    fld1
    fst dword ptr [rcx+14h]
    fstp dword ptr [rcx+3Ch]
    fst dword ptr [rcx+18h]
    fst dword ptr [rcx+1Ch]
    fst dword ptr [rcx+24h]
    fst dword ptr [rcx+2Ch]
    fst dword ptr [rcx+30h]
    fst dword ptr [rcx+34h]
    fstp dword ptr [rcx+38h]
    fstp dword ptr [rcx+20h]
    fstp dword ptr [rcx+28h]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [rsp+16]
@@:
    mov rax, rcx
    ret
PcD3dxMatrixRotationY ENDP

PcD3dxMatrixRotationZ PROC
    movss dword ptr [rsp+8], xmm1
    fnstcw word ptr [rsp+16]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [PcD3dxX87Cw]
@@:
    fld dword ptr [rsp+8]
    fsincos
    fstp dword ptr [rsp+24]
    fstp dword ptr [rsp+28]
    fld dword ptr [rsp+24]
    fst dword ptr [rcx]
    fld dword ptr [rsp+28]
    fst dword ptr [rcx+4h]
    fldz
    fst dword ptr [rcx+8h]
    fst dword ptr [rcx+0Ch]
    fxch st(1)
    fchs
    fstp dword ptr [rcx+10h]
    fxch st(1)
    fstp dword ptr [rcx+14h]
    fst dword ptr [rcx+18h]
    fst dword ptr [rcx+1Ch]
    fst dword ptr [rcx+20h]
    fst dword ptr [rcx+24h]
    fld1
    fst dword ptr [rcx+28h]
    fstp dword ptr [rcx+3Ch]
    fst dword ptr [rcx+2Ch]
    fst dword ptr [rcx+30h]
    fst dword ptr [rcx+34h]
    fstp dword ptr [rcx+38h]
    cmp word ptr [rsp+16], 027Fh
    je @F
    fldcw word ptr [rsp+16]
@@:
    mov rax, rcx
    ret
PcD3dxMatrixRotationZ ENDP

PcD3dxMatrixRotationAxis PROC FRAME
    push rbx
    .pushreg rbx
    sub rsp, 80
    .allocstack 80
    .endprolog
    movss dword ptr [rsp+112], xmm2
    mov rbx, rcx
    fnstcw word ptr [rsp+72]
    cmp word ptr [rsp+72], 027Fh
    je @F
    fldcw word ptr [PcD3dxX87Cw]
@@:
    fld dword ptr [rsp+112]
    fsincos
    fstp dword ptr [rsp+48]
    fstp dword ptr [rsp+56]
    fld dword ptr [rsp+48]
    fld1
    mov eax, dword ptr [rdx]
    mov dword ptr [rsp+32], eax
    fsubrp st(1), st
    mov eax, dword ptr [rdx+4h]
    mov dword ptr [rsp+36], eax
    fstp dword ptr [rsp+44]
    mov eax, dword ptr [rdx+8h]
    mov dword ptr [rsp+40], eax
    lea rcx, [rsp+32]
    mov rdx, rcx
    call PcD3dxVec3Normalize
    fld dword ptr [rsp+36]
    fld st(0)
    fld dword ptr [rsp+32]
    fld st(0)
    fmulp st(2), st
    fld dword ptr [rsp+44]
    fld st(0)
    fmulp st(3), st
    fxch st(2)
    fstp dword ptr [rsp+52]
    fld dword ptr [rsp+40]
    fld st(0)
    fld st(4)
    fmulp st(1), st
    fld st(3)
    fmulp st(1), st
    fstp dword ptr [rsp+60]
    fld st(0)
    fld st(2)
    fmulp st(1), st
    fld st(3)
    fmulp st(1), st
    fstp dword ptr [rsp+64]
    fxch st(1)
    fmul st(0), st
    fmulp st(2), st
    fld dword ptr [rsp+48]
    faddp st(2), st
    fxch st(1)
    fstp dword ptr [rbx]
    fld dword ptr [rsp+56]
    fld st(0)
    fmulp st(2), st
    fld dword ptr [rsp+52]
    fld st(0)
    fadd st, st(3)
    fstp dword ptr [rbx+4h]
    fld st(1)
    fmulp st(4), st
    fld dword ptr [rsp+64]
    fld st(0)
    fsub st, st(5)
    fstp dword ptr [rbx+8h]
    fldz
    fst dword ptr [rbx+0Ch]
    fxch st(2)
    fsubrp st(4), st
    fxch st(3)
    fstp dword ptr [rbx+10h]
    fld dword ptr [rsp+36]
    fmul st(0), st
    fmul dword ptr [rsp+44]
    fld dword ptr [rsp+48]
    fld st(0)
    faddp st(2), st
    fxch st(1)
    fstp dword ptr [rbx+14h]
    fld dword ptr [rsp+32]
    fmulp st(3), st
    fld dword ptr [rsp+60]
    fld st(0)
    fadd st, st(4)
    fstp dword ptr [rbx+18h]
    fxch st(2)
    fst dword ptr [rbx+1Ch]
    fxch st(4)
    faddp st(5), st
    fxch st(4)
    fstp dword ptr [rbx+20h]
    fsubrp st(1), st
    fstp dword ptr [rbx+24h]
    fld dword ptr [rsp+40]
    fmul st(0), st
    fmul dword ptr [rsp+44]
    faddp st(2), st
    fxch st(1)
    fstp dword ptr [rbx+28h]
    fst dword ptr [rbx+2Ch]
    fst dword ptr [rbx+30h]
    fst dword ptr [rbx+34h]
    fstp dword ptr [rbx+38h]
    fld1
    fstp dword ptr [rbx+3Ch]
    cmp word ptr [rsp+72], 027Fh
    je @F
    fldcw word ptr [rsp+72]
@@:
    mov rax, rbx
    add rsp, 80
    pop rbx
    ret
PcD3dxMatrixRotationAxis ENDP

END
