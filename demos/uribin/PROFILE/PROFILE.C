

unsigned char buffer[64000];


void fdBlur() {
    _asm {
        mov   esi, offset buffer
        mov   ecx, 64000
        @inner:
        mov   al,  [esi - 1]
        add   al,  [esi + 1]
        rcr   al, 1
        mov   [esi], al
        inc   esi
        dec   ecx
        jnz   @inner
    }
}


void fdBlur2();
#pragma aux fdBlur2 = \
    "   mov   esi, offset buffer " \
    "   mov   ecx, 64000 " \
    "   @inner: " \
    "   mov   al,  [esi - 1] " \
    "   add   al,  [esi + 1] " \
    "   rcr   al, 1 " \
    "   mov   [esi], al " \
    "   inc   esi " \
    "   dec   ecx " \
    "   jnz   @inner " \
    modify [eax ecx esi]

void main() {
    int i, j;
    
    // prefetch
    //for (i = 0; i < 64000; i++) j = buffer[i];
    
    for (i = 0; i < 16; i++) fdBlur();
    //for (i = 0; i < 16; i++) fdBlur2();
}