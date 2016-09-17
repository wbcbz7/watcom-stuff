#include <strings.h>
#include <memory.h>

//#define BUFSIZE 262144
#define BUFSIZE 1024

unsigned char src[BUFSIZE], dst[BUFSIZE];

void t_memcpy(void *dst, void *src, unsigned long len) {
    memcpy(dst, src, len);
}

void t_repmovsd(void *dst, void *src, unsigned long len) {
    _asm {
        cld
        mov   esi, src
        mov   edi, dst
        mov   ecx, len
        mov   eax, ecx
        shr   ecx, 2
        
        rep   movsd
        
        and   eax, 3
        jz    __skip
        mov   ecx, eax
        rep   movsb
        __skip:
    }
}

void t_iunroll(void *dst, void *src, unsigned long len) {
    _asm {
        cld
        mov   esi, src
        mov   edi, dst
        mov   ecx, len
        push  ecx
        shr   ecx, 3   // copy 8 bytes at once
        
        __loop:
        mov   eax, [esi]
        mov   ebx, [esi+4]
        mov   [edi], eax
        mov   [edi+4], ebx
        add   esi, 8
        add   edi, 8
        dec   ecx
        jnz   __loop
        
        pop   ecx
        and   ecx, 7
        jz    __skip
        rep   movsb
        __skip:
    }
}

void t_iunroll2(void *dst, void *src, unsigned long len) {
    _asm {
        cld
        mov   esi, src
        mov   edi, dst
        mov   ecx, len
        push  ebp
        push  ecx
        mov   ebp, ecx
        shr   ebp, 4   // copy 16 bytes at once
        
        __loop:
        mov   eax, [esi]
        mov   ebx, [esi+4]
        mov   ecx, [esi+8]
        mov   edx, [esi+12]
        mov   [edi], eax
        mov   [edi+4], ebx
        mov   [edi+8], ecx
        mov   [edi+12], edx
        add   esi, 16
        add   edi, 16
        dec   ebp
        jnz   __loop
        
        pop   ecx
        and   ecx, 15
        jz    __skip
        rep   movsb
        __skip:
        pop   ebp
    }
}

void t_fpu(void *dst, void *src, unsigned long len) {
    _asm {
        cld
        mov   esi, src
        mov   edi, dst
        mov   ecx, len
        push  ecx
        shr   ecx, 5   // copy 32 bytes at once
        
        __loop:
        fild  qword ptr [esi]     // a
        fild  qword ptr [esi+8]   // b a
        fild  qword ptr [esi+16]  // c b a
        fxch  st(2)               // a b c
        fild  qword ptr [esi+24]  // d a b c 
        fxch  st(2)               // b a d c
        
        fistp qword ptr [edi+8]   // a d c
        fistp qword ptr [edi]     // d c
        fistp qword ptr [edi+24]  // c
        fistp qword ptr [edi+16]
        
        add   esi, 32
        add   edi, 32
        dec   ecx
        jnz   __loop
        
        pop   ecx
        and   ecx, 32
        jz    __skip
        rep   movsb
        __skip:
    }
}

int main() {
    // prewarm
    memcpy(&dst, &src, BUFSIZE);
    
    // do test
    t_memcpy(&dst, &src, BUFSIZE);
    t_repmovsd(&dst, &src, BUFSIZE);
    t_iunroll(&dst, &src, BUFSIZE);
    t_iunroll2(&dst, &src, BUFSIZE);
    t_fpu(&dst, &src, BUFSIZE);
    
    return 0;
};