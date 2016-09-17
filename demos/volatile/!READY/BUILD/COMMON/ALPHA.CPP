
#define sat(a, l) (a < l ? a : l)

unsigned short *alignbuf;
unsigned short *alphatab;

void initalpha() {
    alignbuf    = new unsigned short[65536 + 33792];
    alphatab    = (unsigned short*)((((unsigned int)alignbuf)  + 0x1FFFF) & ~0x1FFFF);
    
    unsigned short *p = alphatab;
    
    // R*G*A 15bpp table
    for (int r = 0; r < 32; r++) {
        for (int g = 0; g < 32; g++) {
            for (int a = 0; a < 32; a++) {
                int ar = (r * a) >> 5;
                int ag = (g * a) >> 5;
                
                *p++ = (ar << 10) | (ag << 5);
            }
        }
    }
    
    // A*B 15bpp table
    for (int a = 0; a < 32; a++) {
        for (int b = 0; b < 32; b++) {
            *p++ = (b * a) >> 5;
        }
    }
}

void donealpha() {
    delete[] alignbuf;
}

void alphablend(unsigned short *dst, unsigned short *src, unsigned int a) {
    unsigned short *p = src;
    unsigned short *v = dst;
    unsigned short *t = (unsigned short*)((unsigned int)alphatab >> 1);
    
    unsigned int alpha = sat(a, 31);
    static unsigned int sa1, sa2, da1, da2;
    
    /*
    sa1   = alpha * 0x00010001;        sa2 = ((alpha << 5) | 0x8000) * 0x00010001;
    da1   = (31 - alpha) * 0x00010001; da2 = (((31 - alpha) << 5) | 0x8000) * 0x00010001;
    */
    
    sa1   = alpha;        sa2 = ((alpha << 5) | 0x8000);
    da1   = (31 - alpha); da2 = (((31 - alpha) << 5) | 0x8000);
    
    // res = src*alpha + dst*(31-alpha)
    
    _asm {
        mov     esi, p          // 1
        mov     edi, v          // .
        mov     ecx, t          // 2
        push    ebp             // .
        mov     ebp, 64000      // 3
        
        _loop:
        mov     ax, [esi]       // 1
        add     esi, 2          // .
        mov     bx, [edi]       // 2
        mov     edx, eax        // .
        and     eax, 0x00007FE0 // 3
        and     edx, 0x0000001F // .
        or      eax, [sa1]      // 4
        or      edx, [sa2]      // .
        mov     cx, ax          // 5
        add     edi, 2          // .
        mov     ax, [2*ecx]     // 6
        mov     cx, dx          // .
        mov     edx, ebx        // 7
        and     ebx, 0x00007FE0 // .
        or      ax, [2*ecx]     // 8
        or      ebx, [da1]      // .
        mov     cx,  bx         // 9
        and     edx, 0x0000001F // .
        mov     bx, [2*ecx]     // 10
        or      edx, [da2]      // .
        mov     cx, dx          // 11
        add     eax, ebx        // .
        mov     dx, [2*ecx]     // 12
        add     eax, edx        // 13 (stall)
        dec     ebp             // .
        mov     [edi-2], ax     // 14
        jnz     _loop           // .
        pop     ebp
    }
    
    // "конечно, 4 лукапа по табличке многовато, но таки realtime :)"
}

void alphafade(unsigned short *dst, unsigned short color, unsigned int a) {
    unsigned short *v  = dst;
    unsigned short *t  = (unsigned short*)((unsigned int)alphatab >> 1);
    
    unsigned int alpha = sat(a, 31);
    static unsigned int sa1, sa2, da1, da2;
    unsigned int bcol, col;
    
    col = color;
    
    sa1   = alpha;        sa2 = ((alpha << 5) | 0x8000);
    da1   = (31 - alpha); da2 = (((31 - alpha) << 5) | 0x8000);
    
    bcol = ((alphatab[da1 | (col & 0x7FE0)]) | (alphatab[da2 | (col & 0x1F)])) & 0x7FFF;
    
    // res = dst*alpha + bcol*(31-alpha)
    
    _asm {
        mov     edi, v          // 1
        mov     ecx, t          // .
        mov     ebx, bcol       // 2
        push    ebp
        mov     ebp, 64000      // 3
        
        _loop:
        mov     ax, [edi]       // 1
        add     edi, 2          // .
        
        mov     edx, eax        // 2
        and     eax, 0x00007FE0 // .
        
        and     edx, 0x0000001F // 3
        or      eax, [sa1]      // .
        
        or      edx, [sa2]      // 4
        mov     cx, ax          // .
        
        mov     ax, [2*ecx]     // 5-6
        mov     cx, dx          // .
        
        or      ax, [2*ecx]     // 7-8
        
        add     eax, ebx        // 9
        dec     ebp             // .
        
        mov     [edi-2], ax     // 10
        jnz     _loop           // .
        
        pop     ebp
    }
}