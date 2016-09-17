
// 16.16 fixedpoint affine texture mapper, 8bpp
// from fatmap2.txt
// texture must be aligned to 64k boundary
void tmap16_8bpp(unsigned char *dst, unsigned char *texture, unsigned int len,
                 unsigned int u,  unsigned int v,
                 unsigned int du, unsigned int dv) {

    int t_eax, t_ebx, t_ecx, t_edx, t_esi, t_edi, t_ebp;
    
    t_eax =  (u  & 0xFFFF) << 16;
    t_ebx = ((unsigned long)texture & 0xFFFF0000) |
            ((u  & 0xFF0000) >> 16) |
            ((v  & 0xFF0000) >> 8);
    t_ecx = len;
    t_edx = ((v  & 0xFFFF) << 16) | 
            ((du & 0xFF0000) >> 16) |
            ((dv & 0xFF0000) >> 8);
    t_esi =  (du & 0xFFFF) << 16;
    t_edi =  (unsigned long)dst;
    t_ebp =  (dv & 0xFFFF) << 16;
    
    _asm {
        push    ebp
        mov     eax, t_eax
        mov     ebx, t_ebx
        mov     ecx, t_ecx
        mov     edx, t_edx
        mov     esi, t_esi
        mov     edi, t_edi
        mov     ebp, t_ebp
        
    // bitmap (256x256) must be aligned on 64k. (low 16 bits zero)
    // eax =  u frac           : -
    // ebx =  bitmap ptr       : v int      : u int
    // ecx =  scanline width
    // edx =  v frac           : v int step : u int step
    // esi =  u frac step      : 0          : 0
    // edi =  destination ptr
    // ebp =  v frac step      : 0          : 0
    
        lea     edi, [edi+ecx]
        xor     ecx, -1
        inc     ecx
        
        inner:
        mov     al, [ebx]      // get color
        add     edx, ebp       // v frac += v frac step
        adc     bh, dh         // v int  += v int step (+carry from v frac)
        add     eax, esi       // u frac += u frac step
        adc     bl, dl         // u int  += u int step (+carry from u frac)
        mov     [edi+ecx], al  // plot pixel
        inc     ecx
        jnz     inner
        
        pop     ebp
    }
    // 4 cycles\pixel, 100% pairing
}

// 24.8 fixedpoint affine texture mapper, 8bpp
// written by me :)
void tmap8_8bpp(unsigned char *dst, unsigned char *texture, unsigned int len,
                unsigned int u,  unsigned int v,
                unsigned int du, unsigned int dv) {

    static unsigned int t_du, t_dv;
    
    t_du = du; t_dv = dv;
    
    _asm {
        xor     eax, eax
        mov     ebx, u
        mov     edx, v
        mov     ecx, len
        mov     esi, texture
        mov     edi, dst
        
        _loop:
        mov     ah, bh              // 1
        inc     edi                 // .
        mov     al, dh              // 2
        add     ebx, t_du           // .
        mov     al, [esi + eax]     // 3
        add     edx, t_dv           // .
        mov     [edi - 1], al       // 4
        dec     ecx                 // .
        jnz     _loop               // 5
    }
    // 4.5 cycles\pixel, ~100% pairing
}
