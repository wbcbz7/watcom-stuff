
void ptc_fastmemcpy(void *dst, void *src, unsigned long len) {
    _asm {
        cld
        mov   esi, src
        mov   edi, dst
        mov   ecx, len
        push  ebp
        push  ecx
        mov   ebp, ecx
        shr   ebp, 4   // copy 16 bytes at once
        jz    __tail
        
        // prefetch
        mov   al, [esi]
        mov   bl, [esi+32]
        mov   al, [esi+64]
        mov   bl, [esi+96]
        mov   al, [esi+128]
        mov   bl, [esi+160]
        mov   al, [esi+192]
        mov   bl, [esi+224]
        
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
        
        __tail:
        pop   ecx
        and   ecx, 15
        jz    __skip
        rep   movsb
        __skip:
        pop   ebp
    }
}

void ptc_lineartobanked(void *dst, void *src, unsigned long start, unsigned long len) {
    unsigned int i, j,f_length, length = len;
    unsigned int banksize = (ptc_int.modeinfo.Granularity << 10),
                 banknum, bankmask = (banksize - 1),
                 bank     = ((start & ~bankmask) / banksize);
    unsigned long bankedptr = (unsigned long)dst + (start & bankmask);
    unsigned long linearptr = (unsigned long)src;
    
    if ((start & bankmask) != 0) {
        f_length = banksize - (start & bankmask);
        vbe_SetWindowPos(0, bank++);
        ptc_fastmemcpy((void*)bankedptr, (void*)linearptr, f_length);
        linearptr += f_length;
        length -= f_length;
    }
    
    banknum = (length / banksize);
    bankedptr = (unsigned long)dst;
    
    for (i = 0; i < banknum; i++) {
        vbe_SetWindowPos(0, bank++);
        ptc_fastmemcpy((void*)bankedptr, (void*)linearptr, banksize);
        linearptr += banksize;
        length -= banksize;
    }
    
    
    // copy last part
    vbe_SetWindowPos(0, bank++);
    ptc_fastmemcpy((void*)bankedptr, (void*)linearptr, length);
    
}

// pixel format conversion procedures

void ptc_convert_argb8888_abgr8888(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        
        __loop:             //  eax - ebx - edx
        mov    eax, [esi]   //  argb   ?     ?
        mov    ebx, eax     //  argb  argb   ?
        
        shld   edx, eax, 16 //  argb  argb  00ar
        xchg   dl, bl       //  argb  argr  00ab
        shrd   eax, edx, 16 //  abar  argr  00ab
        mov    ax, bx       //  abgr  argr  00ab
        mov    [edi], eax

        add    esi, 4
        add    edi, 4
        
        dec    ecx
        jnz    __loop
    };
}

void ptc_convert_argb8888_rgb888(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        
        __loop:
        movsd
        dec    edi
        dec    ecx
        jnz    __loop
    };
}

void ptc_convert_argb8888_bgr888(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        
        __loop:             //  eax - ebx - edx
        mov    eax, [esi]   //  argb   ?     ?
        mov    ebx, eax     //  argb  argb   ?
        
        shld   edx, eax, 16 //  argb  argb  00ar
        xchg   dl, bl       //  argb  argr  00ab
        shrd   eax, edx, 16 //  abar  argr  00ab
        mov    ax, bx       //  abgr  argr  00ab
        mov    [edi], eax

        add    esi, 4
        add    edi, 3
        
        dec    ecx
        jnz    __loop
    };
}
void ptc_convert_argb8888_rgb565(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        // process two pixels in one loop iteration
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        shr    ecx, 1
        
        __loop:             //  f......ax......0 - ebx - edx
        mov    edx, [esi]   //  0000000000000000  0000  argb
        rol    edx, 8       //  0000000000000000  0000  rgba
        shld   eax, edx, 5  //  00000000000rrrrr  0000  rgba
        add    esi, 4       //  pairing fixup
        rol    edx, 8       //  00000000000rrrrr  0000  gbar
        shld   eax, edx, 6  //  00000rrrrrgggggg  0000  gbar
        rol    edx, 8       //  00000rrrrrgggggg  0000  barg
        shld   eax, edx, 5  //  rrrrrggggggbbbbb  0000  barg

        // second pixel
        mov    edx, [esi]   //  0000000000000000  0000  argb
        rol    edx, 8       //  0000000000000000  0000  rgba
        shld   eax, edx, 5  //  00000000000rrrrr  0000  rgba
        add    esi, 4       //  pairing fixup
        rol    edx, 8       //  00000000000rrrrr  0000  gbar
        shld   eax, edx, 6  //  00000rrrrrgggggg  0000  gbar
        rol    edx, 8       //  00000rrrrrgggggg  0000  barg
        shld   eax, edx, 5  //  rrrrrggggggbbbbb  0000  barg
        
        ror    eax, 16
        mov    [edi], eax        
        add    edi, 4
        
        dec    ecx
        jnz    __loop
    };
}

void ptc_convert_argb8888_rgb555(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        // process two pixels in one loop iteration
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        shr    ecx, 1
        
        __loop:             //  f......ax......0 - ebx - edx
        mov    edx, [esi]   //  0000000000000000  0000  argb
        rol    edx, 8       //  0000000000000000  0000  rgba
        shld   eax, edx, 5  //  00000000000rrrrr  0000  rgba
        add    esi, 4       //  pairing fixup
        rol    edx, 8       //  00000000000rrrrr  0000  gbar
        shld   eax, edx, 5  //  000000rrrrrggggg  0000  gbar
        rol    edx, 8       //  000000rrrrrggggg  0000  barg
        shld   eax, edx, 5  //  0rrrrrgggggbbbbb  0000  barg

        // second pixel
        shl    eax, 1
        mov    edx, [esi]   //  0000000000000000  0000  argb
        rol    edx, 8       //  0000000000000000  0000  rgba
        shld   eax, edx, 5  //  00000000000rrrrr  0000  rgba
        add    esi, 4       //  pairing fixup
        rol    edx, 8       //  00000000000rrrrr  0000  gbar
        shld   eax, edx, 5  //  000000rrrrrggggg  0000  barg
        rol    edx, 8       //  000000rrrrrggggg  0000  barg
        shld   eax, edx, 5  //  0rrrrrgggggbbbbb  0000  barg
        
        ror    eax, 16
        mov    [edi], eax        
        add    edi, 4
        
        dec    ecx
        jnz    __loop
    };
}

void ptc_convert_argb8888_gray8(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        // process 4 pixels per loop iteration
        // used formula: pixel = (r >> 2) + (b >> 2) + (g >> 1);
        mov    esi, inbuf
        mov    edi, outbuf
        mov    ecx, len
        shr    ecx, 2
        
        __loop:           // f......ax......0 - f......bx......0 - edx
        mov    edx, [esi] // 0000000000000000   0000000000000000   argb
        mov    al,  dh    // 00000000gggggggg   0000000000000000   argb
        
        mov    bl,  dl    // 00000000gggggggg   00000000bbbbbbbb   argb
        shr    al,  1     // 000000000ggggggg   0000000000000000   argb
        
        ror    edx, 8     // 000000000ggggggg   00000000bbbbbbbb   barg
        mov    bh,  dh    // 000000000ggggggg   rrrrrrrrbbbbbbbb   barg
        
        shr    ebx, 2     // 000000000ggggggg   00rrrrrrrrbbbbbb   barg
        add    al,  bh    // 00000000cccccccc   00rrrrrrrrbbbbbb   barg
        
        and    bl,  0x3F  // 00000000cccccccc   00rrrrrr00bbbbbb   barg
        add    al,  bl
        
        ror    eax, 8
        add    esi, 4
        
        // 2nd pixel
        
        mov    edx, [esi] // 0000000000000000   0000000000000000   argb
        mov    al,  dh    // 00000000gggggggg   0000000000000000   argb
        
        mov    bl,  dl    // 00000000gggggggg   00000000bbbbbbbb   argb
        shr    al,  1     // 000000000ggggggg   0000000000000000   argb
        
        ror    edx, 8     // 000000000ggggggg   00000000bbbbbbbb   barg
        mov    bh,  dh    // 000000000ggggggg   rrrrrrrrbbbbbbbb   barg
        
        shr    ebx, 2     // 000000000ggggggg   00rrrrrrrrbbbbbb   barg
        add    al,  bh    // 00000000cccccccc   00rrrrrrrrbbbbbb   barg
        
        and    bl,  0x3F  // 00000000cccccccc   00rrrrrr00bbbbbb   barg
        add    al,  bl
        
        ror    eax, 8
        add    esi, 4
        
        // 3rd pixel
        
        mov    edx, [esi] // 0000000000000000   0000000000000000   argb
        mov    al,  dh    // 00000000gggggggg   0000000000000000   argb
        
        mov    bl,  dl    // 00000000gggggggg   00000000bbbbbbbb   argb
        shr    al,  1     // 000000000ggggggg   0000000000000000   argb
        
        ror    edx, 8     // 000000000ggggggg   00000000bbbbbbbb   barg
        mov    bh,  dh    // 000000000ggggggg   rrrrrrrrbbbbbbbb   barg
        
        shr    ebx, 2     // 000000000ggggggg   00rrrrrrrrbbbbbb   barg
        add    al,  bh    // 00000000cccccccc   00rrrrrrrrbbbbbb   barg
        
        and    bl,  0x3F  // 00000000cccccccc   00rrrrrr00bbbbbb   barg
        add    al,  bl
        
        ror    eax, 8
        add    esi, 4
        
        // 4th pixel
        
        mov    edx, [esi] // 0000000000000000   0000000000000000   argb
        mov    al,  dh    // 00000000gggggggg   0000000000000000   argb
        
        mov    bl,  dl    // 00000000gggggggg   00000000bbbbbbbb   argb
        shr    al,  1     // 000000000ggggggg   0000000000000000   argb
        
        ror    edx, 8     // 000000000ggggggg   00000000bbbbbbbb   barg
        mov    bh,  dh    // 000000000ggggggg   rrrrrrrrbbbbbbbb   barg
        
        shr    ebx, 2     // 000000000ggggggg   00rrrrrrrrbbbbbb   barg
        add    al,  bh    // 00000000cccccccc   00rrrrrrrrbbbbbb   barg
        
        and    bl,  0x3F  // 00000000cccccccc   00rrrrrr00bbbbbb   barg
        add    al,  bl
        
        ror    eax, 8
        mov    [edi], eax

        add    esi, 4
        add    edi, 4
        
        dec    ecx
        jnz    __loop
    };
}
