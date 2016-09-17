
long idiv16(long x, long y);        // (x << 16) / y
#pragma aux idiv16 = \
    " mov   edx,eax    "\
    " sar   edx,16     "\
    " shl   eax,16     "\
    " idiv  ebx        "\
    parm [eax] [ebx] modify exact [eax edx] value [eax]

void wf::wulinedraw(vec2f *a, vec2f *b, line *f) {
    int delta_x, delta_y, d;    
    int sx, sy;
    unsigned short *p = buffer;
    static long col, col_r, col_g, col_b;
    static int  adx, ady, dudx, dudy;
    unsigned short *t = wutab;
    
    // if new color then recalc wutab
    //if (col  != f->col) {
        col   = f->col;
        col_r = (col >> 10) & 0x1F;
        col_g = (col >>  5) & 0x1F;
        col_b = (col >>  0) & 0x1F;
        
        for (int i = 256; i >= 1; i--) *t++ = (((col_r * i) >> 8) << 10) | (((col_g * i) >> 8) << 5) | (((col_b * i) >> 8) << 0);
    //}
    
    t = (unsigned short*)((unsigned int)wutab >> 1);
    
    // determine dx and dy
    delta_x = b->x - a->x;
    delta_y = b->y - a->y;
    
    // set up first point
    sx = a->x; sy = a->y;
    p = &buffer[(X_RES * sy) + sx];
    
    adx = abs(delta_x); ady = abs(delta_y);
    
    if (!adx) {
        // vertical line
        dudy = X_RES*sgn(delta_y);
        for (d = 0; d < ady; d++) {
            *p = col; p += dudy;
        }
        return;
    }
    if (!ady) {
        // horizontal line
        dudx = sgn(delta_x);
        for (d = 0; d < adx; d++) {
            *p = col; p += dudx;
        }
        return;
    }
    
    // else draw line
    if (adx > ady) {
        // 0 < |tan(a)| < 1
        dudx = 2*sgn(delta_x);
        dudy = idiv16(delta_y, adx);
        sy <<= 16;
        p = &buffer[sx];
        _asm {
            mov     edi, p      // 1
            mov     ecx, adx    // .
            mov     eax, col    // 2
            mov     ebx, sy     // .
            push    ebp         // 3
            
            _loop:
            mov     edx, ebx        // 1
            mov     ebp, ebx        // .
            sar     edx, 7          // 2
            and     ebp, 0xFFFF0000 // .
            sar     ebp, 9          // 3
            and     edx, 0xFFFFFE00 // .
            add     edx, ebp        // 4
            add     ebx, [dudy]     // .
            mov     [edi + edx], ax // 5
            add     edi, [dudx]     // .
            dec     ecx             // 6            
            jnz     _loop           // .
            
            _endline:
            pop     ebp
        }
    } else {
        // |tan(a)| >= 1
        dudy = 2*X_RES*sgn(delta_y); 
        dudx = idiv16(delta_x, ady);
        sx <<= 16;
        p = &buffer[X_RES*sy];
        _asm {
            mov     edi, p      // 1
            mov     esi, ady    // .
            mov     ebx, sx     // .
            mov     edx, ebx    // 3
            mov     ecx, t
            
            _loop:
            sar     edx, 16
            mov     cl, bh
            add     ebx, [dudx]
            mov     ax, [2*ecx]

            xor     cl, 0xFF
            mov     ax, [2*ecx]
            
            // и вновь продолжается бой...
            
            
            
            add     edi, [dudy]
            mov     edx, ebx
            dec     esi
            jnz     _loop
            
            _endline:
        }
    }
    //*/
}
/* ЗТО ДЕРЬМО РАБОТАТЕ
_asm {
            mov     edi, p      // 1
            mov     esi, ady    // .
            mov     ebx, sx     // .
            mov     edx, ebx    // 3
            mov     ecx, t
            
            _loop:
            sar     edx, 16             // 1
            mov     cl, bh              // .
            add     ebx, [dudx]         // 2
            mov     ax, [2*ecx]         // .
            mov     [edi + 2*edx], ax   // 3
            xor     cl, 0xFF            // .
            mov     ax, [2*ecx]         // 4
            mov     [edi + 2*edx + 2], ax   // 5
            mov     edx, ebx            // .
            add     edi, [dudy]         // 6
            dec     esi                 // .
            jnz     _loop               // 7
            
            _endline:
        }
*/
 
 
/* неработающее говно
_asm {
            mov     edi, p      // 1
            push    ebp
            mov     ebp, ady    // .
            mov     ebx, sx     // .
            mov     edx, ebx    // 3
            mov     ecx, t
            
            _loop:
            sar     edx, 16             // 1
            mov     cl, bh              // .
            add     ebx, [dudx]         // 2
            mov     ax, [2*ecx]         // .
            ror     eax, 16             // 3
            xor     cl, 0xFF            // .
            mov     ax, [2*ecx]         // 4
            lea     esi, [edi+2*edx]    // .
            
            push    eax
            rol     eax, 16
            push    ebx
            push    ecx
            push    edx
            // saturated хуйня
            
            //mov     bx, [esi]           // 1
            mov     ecx, eax            // .
            mov     edx, ebx            // 2
            and     eax, 0x3DEF3DEF     // . <- msb inverted mask
            and     ebx, 0x3DEF3DEF     // 3
            and     edx, 0x42104210     // . <- msb mask
            add     eax, ebx            // 4
            and     ecx, 0x42104210     // .
            mov     ebx, edx            // 5
            or      edx, ecx            // 6 
            and     ebx, ecx            // .
            mov     ecx, edx            // 7
            and     edx, eax            // .
            or      eax, ecx            // 8
            or      edx, ebx            // .
            shr     edx, 4              // 9
            xor     ebx, ebx            // .
            add     edx, 0x3DEF3DEF     // 10 <- add fixup
            //mov     bx, [esi+2]       // .
            xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
            or      eax, edx            // 12 (stall)
            mov     edx, ebx            // .
            //mov     [esi], ax           // 13
            pop     eax                 // .
            mov     ecx, eax            // .
            and     eax, 0x3DEF3DEF     // . <- msb inverted mask
            and     ebx, 0x3DEF3DEF     // 3
            and     edx, 0x42104210     // . <- msb mask
            add     eax, ebx            // 4
            and     ecx, 0x42104210     // .
            mov     ebx, edx            // 5
            or      edx, ecx            // 6 
            and     ebx, ecx            // .
            mov     ecx, edx            // 7
            and     edx, eax            // .
            or      eax, ecx            // 8
            or      edx, ebx            // .
            shr     edx, 4              // 9
            add     edx, 0x3DEF3DEF     // 10 <- add fixup
            xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
            or      eax, edx            // 12 (stall)
            //mov     [esi+2], ax           // 13
            
            pop    edx
            pop     ecx
            pop     ebx
            
            mov     edx, ebx            // 5
            add     edi, [dudy]         // 6
            dec     ebp                 // .
            jnz     _loop               // 7
            
            _endline:
            pop     ebp
        }
*/