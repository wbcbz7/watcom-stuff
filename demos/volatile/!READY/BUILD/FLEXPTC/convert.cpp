
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

// RGB555 -> RGB565 converter (hi, nvidiots! :)
void ptc_convert_rgb555_rgb565(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf   // 1
        mov    ecx, len     // .
        shr    ecx, 1       // 2
        mov    edi, outbuf  // .
        
        // process two pixels in parallel
        _loop:                  // f......ax......0  f......dx......0 (high words are same)
        mov    eax, [esi]       // 0rrrrrgggggbbbbb  ????????????????
        add    esi, 4
        
        mov    edx, eax         // 0rrrrrgggggbbbbb  0rrrrrgggggbbbbb
        and    eax, 0x7FE07FE0  // 0rrrrrggggg00000  0rrrrrgggggbbbbb
        
        shl    eax, 1           // rrrrrggggg000000  0rrrrrgggggbbbbb
        and    edx, 0x001F001F  // rrrrrggggg000000  00000000000bbbbb
        
        or     eax, edx         // rrrrrggggg0bbbbb  00000000000bbbbb
        add    edi, 4
        
        mov    [edi-4], eax
        dec    ecx
        
        jnz    _loop
        // 5.5 cycles, 2.75 cycles\pixel, ~100% pairing
    }
}

// RGB565 -> RGB555 converter (doubt that it really needed, but nevermind...)
void ptc_convert_rgb565_rgb555(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf   // 1
        mov    ecx, len     // .
        shr    ecx, 1       // 2
        mov    edi, outbuf  // .
        
        // process two pixels in parallel
        _loop:                  // f......ax......0  f......dx......0 (high words are same)
        mov    eax, [esi]       // rrrrrggggggbbbbb  ????????????????
        add    esi, 4
        
        mov    edx, eax         // rrrrrggggggbbbbb  rrrrrggggggbbbbb
        and    eax, 0xFFC0FFC0  // rrrrrggggg000000  rrrrrggggggbbbbb
        
        shr    eax, 1           // 0rrrrrggggg00000  rrrrrggggggbbbbb
        and    edx, 0x001F001F  // 0rrrrrggggg00000  00000000000bbbbb
        
        or     eax, edx         // 0rrrrrgggggbbbbb  00000000000bbbbb
        add    edi, 4
        
        mov    [edi-4], eax
        dec    ecx
        
        jnz    _loop
        // 5.5 cycles, 2.75 cycles\pixel, ~100% pairing
    }
}

// RGB555\565 pixel doubler 
void ptc_convert_rgb565_dup(void* outbuf, void* inbuf, unsigned int len) {
    _asm {
        mov    esi, inbuf   // 1
        mov    ecx, len     // .
        shr    ecx, 1       // 2
        mov    edi, outbuf  // .
        
        // process two pixels in parallel
        _loop:                  // eax - ebx - edx 
        mov    eax, [esi]       // 2211  ????  ????
        add    esi, 4
        
        mov    ebx, eax         // 2211  2211  ????
        mov    edx, eax         // 2211  2211  2211
        
        shl    ebx, 16          // 2211  1100  2211
        and    edx, 0x0000FFFF  // 2211  1100  0011
        
        or     edx, ebx         // 2211  1100  1111
        mov    ebx, eax         // 2211  2211  1111
        
        shr    eax, 16          // 0022  2211  1111
        and    ebx, 0xFFFF0000  // 0022  2200  1111  
        
        mov    [edi], edx
        add    edi, 8
        
        or     eax, ebx         // 2222  2200  1111
        dec    ecx
        
        mov    [edi-4], eax
        jnz    _loop
        // 8 cycles, 4 cycles\pixel, 100% pairing
    }
}

