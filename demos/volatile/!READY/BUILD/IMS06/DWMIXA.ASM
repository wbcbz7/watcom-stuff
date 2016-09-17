;// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
;// you must not read, modify, print, compile or copy this file or parts of
;// it unless you have accepted the license in the accompanying file IMS.TXT.

.386
.model flat,prolog
locals

.data

MIXR_PLAYING equ 1
MIXR_PAUSED equ 2
MIXR_LOOPED equ 4
MIXR_PINGPONGLOOP equ 8
MIXR_PLAY16BIT equ 16
MIXR_INTERPOLATE equ 32
MIXR_PLAYSTEREO equ 128

channel struc
  chsamp dd ?
  chlength dd ?
  chloopstart dd ?
  chloopend dd ?
  chreplen dd ?
  chstep dd ?
  chpos dd ?
  chfpos dw ?
  chstatus db ?,?
  chvols dd 4 dup (?)
  chdstvols dd 4 dup (?)
ends

argdb macro name
  org $-1
  name db 0
endm

argdw macro name
  org $-2
  name dw 0
endm

argdd macro name
  org $-4
  name dd 0
endm

public mixrFadeChannel_
mixrFadeChannel_ proc
  mov ebx,[edi].chvols[0]
  mov ecx,[edi].chvols[4]
  shl ebx,8
  shl ecx,8
  mov eax,[edi].chsamp
  add eax,[edi].chpos
  test [edi].chstatus,MIXR_PLAY16BIT
  jnz @@16
    mov bl,[eax]
  jmp @@do
@@16:
    mov bl,[2*eax+1]
@@do:
  mov cl,bl
  mov ebx,[4*ebx+1234]
    argdd @@voltab1
  mov ecx,[4*ecx+1234]
    argdd @@voltab2
  add [esi],ebx
  add [esi+4],ecx
  mov [edi].chvols[0],0
  mov [edi].chvols[4],0
  ret
setupfade:
  mov @@voltab1,eax
  mov @@voltab2,eax
  ret
endp


playquiet proc
  ret
endp

playmono proc
@@lp:
    mov bl,[esi]
    add edx,1234
      argdd monostepl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd monosteph
    add [edi],eax
    add edi,4
    add ebx,1234
      argdd monoramp
  cmp edi,1234
    argdd monoendp
  jb @@lp
  ret

setupmono:
  mov @@vol1,eax
  ret
endp

playmono16 proc
@@lp:
    mov bl,[esi+esi+1]
    add edx,1234
      argdd mono16stepl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd mono16steph
    add [edi],eax
    add edi,4
    add ebx,1234
      argdd mono16ramp
  cmp edi,1234
    argdd mono16endp
  jb @@lp
  ret

setupmono16:
  mov @@vol1,eax
  ret
endp

playmonoi proc
@@lp:
    mov eax,edx
    shr eax,20
    mov al,[esi]
    mov bl,[eax+eax+1234]
      argdd @@int0
    mov al,[esi+1]
    add bl,[eax+eax+1234]
      argdd @@int1

    add edx,1234
      argdd monoistepl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd monoisteph
    add [edi],eax
    add edi,4
    add ebx,1234
      argdd monoiramp
  cmp edi,1234
    argdd monoiendp
  jb @@lp
  ret

setupmonoi:
  mov @@vol1,eax
  mov @@int0,ebx
  inc ebx
  mov @@int1,ebx
  dec ebx
  ret
endp

playmonoi16 proc
@@lp:
    mov eax,edx
    shr eax,20
    mov al,[esi+esi+1]
    mov bl,[eax+eax+1234]
      argdd @@int0
    mov al,[esi+esi+3]
    add bl,[eax+eax+1234]
      argdd @@int1

    add edx,1234
      argdd monoi16stepl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd monoi16steph
    add [edi],eax
    add edi,4
    add ebx,1234
      argdd monoi16ramp
  cmp edi,1234
    argdd monoi16endp
  jb @@lp
  ret

setupmonoi16:
  mov @@vol1,eax
  mov @@int0,ebx
  inc ebx
  mov @@int1,ebx
  dec ebx
  ret
endp



playstereo proc
@@lp:
    mov bl,[esi]
    add edx,1234
      argdd stereostepl
    mov cl,[esi]
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd stereosteph
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
    add ebx,1234
      argdd stereoramp0
    add ecx,1234
      argdd stereoramp1
  cmp edi,1234
    argdd stereoendp
  jb @@lp
  ret

setupstereo:
  mov @@vol1,eax
  mov @@vol2,eax
  ret
endp

playstereo16 proc
@@lp:
    mov bl,[esi+esi+1]
    add edx,1234
      argdd stereo16stepl
    mov cl,[esi+esi+1]
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd stereo16steph
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
    add ebx,1234
      argdd stereo16ramp0
    add ecx,1234
      argdd stereo16ramp1
  cmp edi,1234
    argdd stereo16endp
  jb @@lp
  ret

setupstereo16:
  mov @@vol1,eax
  mov @@vol2,eax
  ret
endp

playstereoi proc
@@lp:
    mov eax,edx
    shr eax,20
    mov al,[esi]
    mov bl,[eax+eax+1234]
      argdd @@int0
    mov al,[esi+1]
    add bl,[eax+eax+1234]
      argdd @@int1

    add edx,1234
      argdd stereoistepl
    mov cl,bl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd stereoisteph
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
    add ebx,1234
      argdd stereoiramp0
    add ecx,1234
      argdd stereoiramp1
  cmp edi,1234
    argdd stereoiendp
  jb @@lp
  ret

setupstereoi:
  mov @@vol1,eax
  mov @@vol2,eax
  mov @@int0,ebx
  inc ebx
  mov @@int1,ebx
  dec ebx
  ret
endp

playstereoi16 proc
@@lp:
    mov eax,edx
    shr eax,20
    mov al,[esi+esi+1]
    mov bl,[eax+eax+1234]
      argdd @@int0
    mov al,[esi+esi+3]
    add bl,[eax+eax+1234]
      argdd @@int1

    add edx,1234
      argdd stereoi16stepl
    mov cl,bl
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd stereoi16steph
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
    add ebx,1234
      argdd stereoi16ramp0
    add ecx,1234
      argdd stereoi16ramp1
  cmp edi,1234
    argdd stereoi16endp
  jb @@lp
  ret

setupstereoi16:
  mov @@vol1,eax
  mov @@vol2,eax
  mov @@int0,ebx
  inc ebx
  mov @@int1,ebx
  dec ebx
  ret
endp

dummydd dd 0

routq   dd offset playquiet,    offset dummydd,       offset dummydd,       dummydd,       dummydd,       dummydd,      0,0
routtab dd offset playmono,     offset monostepl,     offset monosteph,     monoramp,      dummydd,       monoendp,     0,0
        dd offset playmono16,   offset mono16stepl,   offset mono16steph,   mono16ramp,    dummydd,       mono16endp,   0,0
        dd offset playmonoi,    offset monoistepl,    offset monoisteph,    monoiramp,     dummydd,       monoiendp,    0,0
        dd offset playmonoi16,  offset monoi16stepl,  offset monoi16steph,  monoi16ramp,   dummydd,       monoi16endp,  0,0
        dd offset playstereo,   offset stereostepl,   offset stereosteph,   stereoramp0,   stereoramp1,   stereoendp,   0,0
        dd offset playstereo16, offset stereo16stepl, offset stereo16steph, stereo16ramp0, stereo16ramp1, stereo16endp, 0,0
        dd offset playstereoi,  offset stereoistepl,  offset stereoisteph,  stereoiramp0,  stereoiramp1,  stereoiendp,  0,0
        dd offset playstereoi16,offset stereoi16stepl,offset stereoi16steph,stereoi16ramp0,stereoi16ramp1,stereoi16endp,0,0

public mixrPlayChannel_
mixrPlayChannel_ proc buf:dword, fadebuf:dword, len:dword, chan:dword, stereo:dword
local routptr:dword,filllen:dword,ramping:dword:2,inloop:byte,ramploop:byte,dofade:byte
  mov edi,chan
  test [edi].chstatus,MIXR_PLAYING
  jz @@exit

  mov filllen,0
  mov dofade,0

  xor eax,eax
  cmp stereo,0
  je @@nostereo
    add eax,4
@@nostereo:
  test [edi].chstatus,MIXR_INTERPOLATE
  jz @@nointr
    add eax,2
@@nointr:
  test [edi].chstatus,MIXR_PLAY16BIT
  jz @@psetrtn
    inc eax
@@psetrtn:
  shl eax,5
  add eax,offset routtab
  mov routptr,eax

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
    test [edi].chstatus,MIXR_LOOPED
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
    test [edi].chstatus,MIXR_LOOPED
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
      and [edi].chstatus,not MIXR_PLAYING
      mov dofade,1
      mov eax,len
      sub eax,ecx
      add filllen,eax
      mov len,ecx

@@playecx:
  mov ramploop,0
  mov ramping[0],0
  mov ramping[4],0

  cmp ecx,0
  je @@noplay

  mov edx,[edi].chdstvols[0]
  sub edx,[edi].chvols[0]
  je @@noramp0
  jl @@ramp0down
    mov ramping[0],1
    cmp ecx,edx
    jbe @@noramp0
      mov ramploop,1
      mov ecx,edx
      jmp @@noramp0
@@ramp0down:
    neg edx
    mov ramping[0],-1
    cmp ecx,edx
    jbe @@noramp0
      mov ramploop,1
      mov ecx,edx
@@noramp0:

  mov edx,[edi].chdstvols[4]
  sub edx,[edi].chvols[4]
  je @@noramp1
  jl @@ramp1down
    mov ramping[4],1
    cmp ecx,edx
    jbe @@noramp1
      mov ramploop,1
      mov ecx,edx
      jmp @@noramp1
@@ramp1down:
    neg edx
    mov ramping[4],-1
    cmp ecx,edx
    jbe @@noramp1
      mov ramploop,1
      mov ecx,edx
@@noramp1:

  mov edx,routptr
  cmp ramping[0],0
  jne @@notquiet
  cmp ramping[4],0
  jne @@notquiet
  cmp [edi].chvols[0],0
  jne @@notquiet
  cmp [edi].chvols[4],0
  jne @@notquiet
    mov edx,offset routq

@@notquiet:
  mov ebx,[edx+4]
  mov eax,[edi].chstep
  shl eax,16
  mov [ebx],eax
  mov ebx,[edx+8]
  mov eax,[edi].chstep
  sar eax,16
  mov [ebx],eax
  mov ebx,[edx+12]
  mov eax,ramping[0]
  shl eax,8
  mov [ebx],eax
  mov ebx,[edx+16]
  mov eax,ramping[4]
  shl eax,8
  mov [ebx],eax
  mov ebx,[edx+20]
  lea eax,[4*ecx]
  cmp stereo,0
  je @@m1
    shl eax,1
@@m1:
  add eax,buf
  mov [ebx],eax

  push ecx
  mov eax,[edx]

  mov ebx,[edi].chvols[0]
  shl ebx,8
  mov ecx,[edi].chvols[4]
  shl ecx,8
  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,buf

  call eax

  pop ecx
  mov edi,chan

@@noplay:
  mov eax,ecx
  shl eax,2
  cmp stereo,0
  je @@m2
    shl eax,1
@@m2:
  add buf,eax
  sub len,ecx

  mov eax,[edi].chstep
  imul ecx
  shld edx,eax,16
  add [edi].chfpos,ax
  adc [edi].chpos,edx

  mov eax,ramping[0]
  imul eax,ecx
  add [edi].chvols[0],eax
  mov eax,ramping[4]
  imul eax,ecx
  add [edi].chvols[4],eax

  cmp ramploop,0
  jnz @@bigloop

  cmp inloop,0
  jz @@fill

  mov eax,[edi].chpos
  cmp [edi].chstep,0
  jge @@forward2
    cmp eax,[edi].chloopstart
    jge @@exit
    test [edi].chstatus,MIXR_PINGPONGLOOP
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
    test [edi].chstatus,MIXR_PINGPONGLOOP
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
  jmp @@exit

@@fill:
  cmp filllen,0
  je @@fadechk
  mov eax,[edi].chlength
  mov [edi].chpos,eax
  add eax,[edi].chsamp
  mov ebx,[edi].chvols[0]
  mov ecx,[edi].chvols[4]
  shl ebx,8
  shl ecx,8
  test [edi].chstatus,MIXR_PLAY16BIT
  jnz @@fill16
    mov bl,[eax]
    jmp @@filldo
@@fill16:
    mov bl,[eax+eax+1]
@@filldo:
  mov cl,bl
  mov ebx,[4*ebx+1234]
    argdd @@voltab1
  mov ecx,[4*ecx+1234]
    argdd @@voltab2
  mov eax,filllen
  mov edi,buf
  cmp stereo,0
  jne @@fillstereo
@@fillmono:
    add [edi],ebx
    add edi,4
  dec eax
  jnz @@fillmono
  jmp @@fade
@@fillstereo:
    add [edi],ebx
    add [edi+4],ecx
    add edi,8
  dec eax
  jnz @@fillstereo
  jmp @@fade

@@fadechk:
  cmp dofade,0
  je @@exit
@@fade:
  mov edi,chan
  mov esi,fadebuf
  call mixrFadeChannel_

@@exit:
  ret

setupplay:
  mov @@voltab1,eax
  mov @@voltab2,eax
  retn
endp

public mixrSetupAddresses_
mixrSetupAddresses_ proc
  call setupfade
  call setupplay
  call setupmono
  call setupmono16
  call setupmonoi
  call setupmonoi16
  call setupstereo
  call setupstereo16
  call setupstereoi
  call setupstereoi16
  ret
endp

public mixrFade_
mixrFade_ proc
  mov eax,[esi]
  mov ebx,[esi+4]
  cmp edx,0
  jnz @@stereo
  @@lpm:
      mov [edi],eax
      mov edx,eax
      shl eax,7
      sub eax,edx
      sar eax,7
      add edi,4
    dec ecx
    jnz @@lpm
  jmp @@done
@@stereo:
  @@lps:
      mov [edi],eax
      mov [edi+4],ebx
      mov edx,eax
      shl eax,7
      sub eax,edx
      sar eax,7
      mov edx,ebx
      shl ebx,7
      sub ebx,edx
      sar ebx,7
      add edi,8
    dec ecx
    jnz @@lps
@@done:
  mov [esi],eax
  mov [esi+4],ebx
  ret
endp

;//*********************************************************************

mixrClip8_ proc ;//esi=src, edi=dst, ebx=tab, ecx=len, edx=max
  mov @@amp1,ebx
  add ebx,512
  mov @@amp2,ebx
  add ebx,512
  mov @@amp3,ebx
  sub ebx,1024
  mov @@max,edx
  neg edx
  mov @@min,edx

  xor edx,edx
  mov dl,byte ptr @@min
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@min+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@min+2
  add eax,[ebx+1024+2*edx]
  mov @@minv,ah
  mov dl,byte ptr @@max
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@max+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@max+2
  add eax,[ebx+1024+2*edx]
  mov @@maxv,ah
  lea ecx,[ecx+edi]
  mov @@endp1,ecx
  mov @@endp2,ecx
  mov @@endp3,ecx
  xor ebx,ebx
  xor ecx,ecx
  xor edx,edx

@@lp:
    cmp dword ptr [esi],1234
      argdd @@min
    jl @@low
    cmp dword ptr [esi],1234
      argdd @@max
    jg @@high

    mov bl,[esi]
    mov cl,[esi+1]
    mov dl,[esi+2]
    mov eax,[2*ebx+1234]
      argdd @@amp1
    add eax,[2*ecx+1234]
      argdd @@amp2
    add eax,[2*edx+1234]
      argdd @@amp3
    mov [edi],ah
    inc edi
    add esi,4
  cmp edi,1234
    argdd @@endp1
  jb @@lp
@@done:
  ret

@@low:
    mov byte ptr [edi],12
      argdb @@minv
    inc edi
    add esi,4
  cmp edi,1234
    argdd @@endp2
  jb @@lp
  jmp @@done
@@high:
    mov byte ptr [edi],12
      argdb @@maxv
    inc edi
    add esi,4
  cmp edi,1234
    argdd @@endp3
  jb @@lp
  jmp @@done
endp

public mixrClip_
mixrClip_ proc ;//esi=src, edi=dst, ebx=tab, ecx=len, edx=max, eax=16bit
  cmp eax,0
  je mixrClip8_

  mov @@amp1,ebx
  add ebx,512
  mov @@amp2,ebx
  add ebx,512
  mov @@amp3,ebx
  sub ebx,1024
  mov @@max,edx
  neg edx
  mov @@min,edx

  xor edx,edx
  mov dl,byte ptr @@min
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@min+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@min+2
  add eax,[ebx+1024+2*edx]
  mov @@minv,ax
  mov dl,byte ptr @@max
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@max+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@max+2
  add eax,[ebx+1024+2*edx]
  mov @@maxv,ax
  lea ecx,[2*ecx+edi]
  mov @@endp1,ecx
  mov @@endp2,ecx
  mov @@endp3,ecx
  xor ebx,ebx
  xor ecx,ecx
  xor edx,edx

@@lp:
    cmp dword ptr [esi],1234
      argdd @@min
    jl @@low
    cmp dword ptr [esi],1234
      argdd @@max
    jg @@high

    mov bl,[esi]
    mov cl,[esi+1]
    mov dl,[esi+2]
    mov eax,[2*ebx+1234]
      argdd @@amp1
    add eax,[2*ecx+1234]
      argdd @@amp2
    add eax,[2*edx+1234]
      argdd @@amp3
    mov [edi],ax
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp1
  jb @@lp
@@done:
  ret

@@low:
    mov word ptr [edi],1234
      argdw @@minv
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp2
  jb @@lp
  jmp @@done
@@high:
    mov word ptr [edi],1234
      argdw @@maxv
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp3
  jb @@lp
  jmp @@done
endp

end
