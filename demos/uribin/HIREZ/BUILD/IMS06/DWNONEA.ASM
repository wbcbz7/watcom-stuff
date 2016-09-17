;// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
;// you must not read, modify, print, compile or copy this file or parts of
;// it unless you have accepted the license in the accompanying file IMS.TXT.

.386
.model flat,prolog
locals

MCP_PLAYING equ 1
MCP_LOOPED equ 4
MCP_PINGPONGLOOP equ 8

channel struc
  chsamp dd ?
  chlength dd ?
  chloopstart dd ?
  chloopend dd ?
  chreplen dd ?
  chstep dd ?
  chpos dd ?
  chfpos dw ?
  chstatus dw ?
ends

.code

public nonePlayChannel_

nonePlayChannel_ proc len:dword, chan:dword
local inloop:byte

  mov edi,chan

@@bigloop:
  mov ecx,len
  mov ebx,[edi].chstep
  mov edx,[edi].chpos
  mov si,[edi].chfpos
  mov inloop,0
  cmp ebx,0
  je @@playecx
  jg @@forward
    neg ebx
    mov eax,edx
    test [edi].chstatus,MCP_LOOPED
    jz @@maxplaylen
    cmp edx,[edi].chloopstart
    jb @@maxplaylen
    sub eax,[edi].chloopstart
    mov inloop,1
    jmp @@maxplaylen
@@forward:
    mov eax,[edi].chlength
    neg si
    sbb eax,edx
    test [edi].chstatus,MCP_LOOPED
    jz @@maxplaylen
    cmp edx,[edi].chloopend
    jae @@maxplaylen
    sub eax,[edi].chlength
    add eax,[edi].chloopend
    mov inloop,1

@@maxplaylen:
  xor edx,edx
  shld edx,eax,16
  shl esi,16
  shld eax,esi,16
  add eax,ebx
  adc edx,0
  sub eax,1
  sbb edx,0
  cmp edx,ebx
  jae @@playecx
  div ebx
  cmp ecx,eax
  jb @@playecx
    mov ecx,eax
    cmp inloop,0
    jnz @@playecx
      and [edi].chstatus,not MCP_PLAYING
      mov len,ecx

@@playecx:
  sub len,ecx

  mov eax,[edi].chstep
  imul ecx
  shld edx,eax,16
  add [edi].chfpos,ax
  adc [edi].chpos,edx
  mov eax,[edi].chpos

  cmp inloop,0
  jz @@exit

  cmp [edi].chstep,0
  jge @@forward2
    cmp eax,[edi].chloopstart
    jge @@exit
    test [edi].chstatus,MCP_PINGPONGLOOP
    jnz @@pong
      add eax,[edi].chreplen
      jmp @@loopiflen
    @@pong:
      neg [edi].chstep
      neg [edi].chfpos
      adc eax,0
      neg eax
      add eax,[edi].chloopstart
      add eax,[edi].chloopstart
      jmp @@loopiflen
@@forward2:
    cmp eax,[edi].chloopend
    jb @@exit
    test [edi].chstatus,MCP_PINGPONGLOOP
    jnz @@ping
      sub eax,[edi].chreplen
      jmp @@loopiflen
    @@ping:
      neg [edi].chstep
      neg [edi].chfpos
      adc eax,0
      neg eax
      add eax,[edi].chloopend
      add eax,[edi].chloopend

@@loopiflen:
  mov [edi].chpos,eax
  cmp len,0
  jne @@bigloop

@@exit:
  ret
endp

end
